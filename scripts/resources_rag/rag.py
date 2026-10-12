"""Local Literature RAG Engine Runner & CLI."""

from __future__ import annotations

from pathlib import Path
from typing import Annotated

import typer
from chunking import extract_chunks_from_document
from embedding import (
    build_chunk_record,
    collect_embeddings,
    execute_search,
    get_embedding_model,
    get_vector_db,
    has_table,
    persist_records,
)
from fastembed import TextEmbedding
from models import (
    BGE_QUERY_PREFIX,
    BIB_PATH,
    COLLECTION_NAME,
    DB_PATH,
    BookMetadata,
    discover_book_files,
    load_bib_metadata,
    resolve_book_metadata,
)
from rich.progress import (
    BarColumn,
    Progress,
    TaskID,
    TextColumn,
    TimeElapsedColumn,
    TimeRemainingColumn,
)

app = typer.Typer(
    add_completion=False,
    help="Dynamic, offline Literature RAG for systems and garbage collection textbooks.",
)


def filter_files(files: list[Path], book_filter: str | None) -> list[Path]:
    """Filter files by book slug or exit if none match."""
    if not book_filter:
        return files
    filtered = [f for f in files if book_filter.lower() in f.stem.lower()]
    if not filtered:
        typer.secho(f"No books matched filter '{book_filter}'.", fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)
    return filtered


def process_single_book(
    file_path: Path,
    meta: BookMetadata,
    model: TextEmbedding,
    progress: Progress | None = None,
    chunk_task: TaskID | None = None,
) -> list[dict[str, object]]:
    """Extract and embed all chunks for a single book with optional progress tracking."""
    if progress and chunk_task is not None:
        desc = f"[cyan]Extracting {file_path.name[:25]}..."
        progress.update(chunk_task, description=desc, total=None, completed=0)

    chunks = extract_chunks_from_document(file_path, meta)
    if not chunks:
        if progress and chunk_task is not None:
            desc = f"[yellow]No chunks: {file_path.name[:25]}"
            progress.update(chunk_task, description=desc, total=1, completed=1)
        return []

    embed_texts = [
        f"Title: {c.title}{' | Section: ' + c.section if c.section else ''}\n\n{c.text}"
        for c in chunks
    ]

    if progress and chunk_task is not None:
        desc = f"[cyan]Embedding {file_path.stem[:25]} ({len(chunks)} chunks)"
        progress.update(chunk_task, description=desc, total=len(chunks), completed=0)

    embeddings = collect_embeddings(model, embed_texts, progress, chunk_task)
    return [build_chunk_record(c, e) for c, e in zip(chunks, embeddings, strict=True)]


@app.command()
def index(
    book: Annotated[
        str | None,
        typer.Option("--book", "-b", help="Index only a specific book slug"),
    ] = None,
) -> None:
    """Index literature files into local LanceDB vector database."""
    files = filter_files(discover_book_files(), book)
    typer.secho(f"Discovered {len(files)} literature files for indexing.", fg=typer.colors.CYAN)

    catalog = load_bib_metadata(BIB_PATH)
    model = get_embedding_model()
    db = get_vector_db()
    all_records: list[dict[str, object]] = []

    with Progress(
        TextColumn("[bold blue]{task.description}"),
        BarColumn(),
        TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
        TextColumn("({task.completed}/{task.total})"),
        TimeElapsedColumn(),
        TimeRemainingColumn(),
    ) as progress:
        book_task = progress.add_task("[bold green]Books", total=len(files))
        chunk_task = progress.add_task("[bold cyan]Chunks", total=0)

        for file_path in files:
            meta = resolve_book_metadata(file_path, catalog)
            book_records = process_single_book(
                file_path, meta, model, progress=progress, chunk_task=chunk_task
            )
            all_records.extend(book_records)
            progress.advance(book_task)

    if not all_records:
        typer.secho("No chunks extracted to index.", fg=typer.colors.YELLOW)
        return

    typer.secho(f"Persisting {len(all_records)} vectors into LanceDB...", fg=typer.colors.CYAN)
    target_slug = files[0].stem if book and files else None
    persist_records(db, all_records, book, target_slug)

    typer.secho(
        f"✅ Indexing complete! Indexed {len(files)} books ({len(all_records)} chunks).",
        fg=typer.colors.GREEN,
        bold=True,
    )


@app.command()
def search(
    query: Annotated[str, typer.Argument(help="Search query text")],
    book: Annotated[
        str | None,
        typer.Option("--book", "-b", help="Filter by book slug substring"),
    ] = None,
    limit: Annotated[int, typer.Option("--limit", "-k", help="Maximum results to return")] = 5,
) -> None:
    """Perform hybrid semantic & full-text search across indexed literature."""
    db = get_vector_db()
    if not has_table(db, COLLECTION_NAME):
        typer.secho(
            "Vector database not found. Run 'just rag-index' first.",
            fg=typer.colors.RED,
            err=True,
        )
        raise typer.Exit(code=1)

    table = db.open_table(COLLECTION_NAME)
    model = get_embedding_model()

    prefixed_query = f"{BGE_QUERY_PREFIX}{query}"
    query_vec = list(model.embed([prefixed_query]))[0].tolist()
    results = execute_search(table, query, query_vec, book, limit)

    if not results:
        typer.secho(f"No results found for query: '{query}'", fg=typer.colors.YELLOW)
        return

    typer.secho(f"\nTop {len(results)} matches for: '{query}'\n", fg=typer.colors.CYAN, bold=True)
    for i, item in enumerate(results, start=1):
        rel = item.get("_relevance_score")
        dist = item.get("_distance", 0.0)
        dist_f = float(str(dist)) if dist is not None else 0.0
        score_val = float(str(rel)) if rel is not None else (1.0 - dist_f)
        page_info = f"Page {item['page']}"
        page_end = item.get("page_end")
        if page_end and page_end != item["page"]:
            page_info = f"Pages {item['page']}–{page_end}"
        section_str = f" | {item['section']}" if item.get("section") else ""
        typer.secho(
            f"[{i}] {item['title']} ({page_info}{section_str}) — Score: {score_val:.4f}",
            fg=typer.colors.GREEN,
            bold=True,
        )
        text_preview = str(item["text"])
        if len(text_preview) > 300:
            text_preview = text_preview[:300] + "..."
        typer.echo(f"    {text_preview}\n")


@app.command()
def stats() -> None:
    """Display statistics about the indexed literature."""
    db = get_vector_db()
    if not has_table(db, COLLECTION_NAME):
        typer.secho("Database empty. Run index first.", fg=typer.colors.YELLOW)
        return

    tbl = db.open_table(COLLECTION_NAME)
    total_records = len(tbl)
    typer.secho(f"Total indexed chunks: {total_records}", fg=typer.colors.GREEN, bold=True)
    typer.echo(f"Database location: {DB_PATH}")


if __name__ == "__main__":
    app()
