#!/usr/bin/env python3
"""Local Literature RAG Engine.

Provides dynamic, metadata-driven offline indexing and semantic search over
systems programming and GC literature in resources/ using PyMuPDF, FastEmbed,
LanceDB, and references.bib.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Annotated

import lancedb
import pymupdf
import typer
from fastembed import TextEmbedding

app = typer.Typer(
    add_completion=False,
    help="Dynamic, offline Literature RAG for systems and garbage collection textbooks.",
)

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent
RESOURCES_DIR: Path = WORKSPACE_ROOT / "resources"
BIB_PATH: Path = RESOURCES_DIR / "references.bib"
DB_PATH: Path = WORKSPACE_ROOT / ".rag_db"
COLLECTION_NAME: str = "literature"
EMBEDDING_MODEL_NAME: str = "BAAI/bge-small-en-v1.5"
SUPPORTED_EXTENSIONS: set[str] = {".pdf", ".epub"}


@dataclass
class BookMetadata:
    """Metadata describing a book from references.bib or filesystem."""

    citation_key: str
    title: str
    author: str
    year: str
    filename: str

    @property
    def display_name(self) -> str:
        """Return formatted title with author and year if available."""
        if self.author and self.year:
            return f"{self.title} ({self.author}, {self.year})"
        return self.title


@dataclass
class TextChunk:
    """A chunk of text extracted from a textbook."""

    id: str
    book_slug: str
    title: str
    author: str
    year: str
    citation_key: str
    page: int
    chunk_index: int
    text: str


def _clean_bib_val(val: str | None) -> str:
    if not val:
        return ""
    cleaned = val.strip().strip("{}").replace("\n", " ")
    return re.sub(r"\s+", " ", cleaned)


def parse_bib_entry(entry: str) -> tuple[str, BookMetadata] | None:
    """Parse a single BibTeX entry into a BookMetadata record."""
    if not entry.strip():
        return None
    key_match = re.search(r"@\w+\s*\{\s*([^,\s]+)", entry)
    file_match = re.search(r"file\s*=\s*\{([^}]+)\}", entry)
    if not key_match or not file_match:
        return None

    key = key_match.group(1).strip()
    fn = Path(file_match.group(1).strip()).name

    title_m = re.search(r"title\s*=\s*\{([^}]+)\}", entry)
    author_m = re.search(r"author\s*=\s*\{([^}]+)\}", entry)
    year_m = re.search(r"year\s*=\s*\{([^}]+)\}", entry)

    return fn, BookMetadata(
        citation_key=key,
        title=_clean_bib_val(title_m.group(1) if title_m else fn),
        author=_clean_bib_val(author_m.group(1) if author_m else ""),
        year=_clean_bib_val(year_m.group(1) if year_m else ""),
        filename=fn,
    )


def load_bib_metadata(bib_path: Path) -> dict[str, BookMetadata]:
    """Parse references.bib into a mapping of filename -> BookMetadata."""
    catalog: dict[str, BookMetadata] = {}
    if not bib_path.is_file():
        return catalog

    content = bib_path.read_text(encoding="utf-8")
    entries = re.split(r"(?=@\w+\s*\{)", content)
    for entry in entries:
        parsed = parse_bib_entry(entry)
        if parsed:
            fn, meta = parsed
            catalog[fn] = meta
    return catalog


def resolve_book_metadata(doc_path: Path, catalog: dict[str, BookMetadata]) -> BookMetadata:
    """Resolve metadata from catalog or synthesize from filename dynamically."""
    if doc_path.name in catalog:
        return catalog[doc_path.name]
    stem = doc_path.stem
    return BookMetadata(
        citation_key=stem,
        title=stem.replace("_", " ").title(),
        author="",
        year="",
        filename=doc_path.name,
    )


def extract_page_chunks(meta: BookMetadata, page_num: int, text: str) -> list[TextChunk]:
    """Split a page's text into overlapping word chunks."""
    cleaned = re.sub(r"\s+", " ", text).strip()
    words = cleaned.split(" ")
    if len(words) < 15:
        return []

    chunk_size = 350
    overlap = 50
    chunks: list[TextChunk] = []
    start = 0
    idx = 0
    book_slug = Path(meta.filename).stem

    while start < len(words):
        end = min(start + chunk_size, len(words))
        chunk_words = words[start:end]
        chunk_text = " ".join(chunk_words)
        chunk_id = f"{book_slug}_p{page_num}_{idx}"
        chunks.append(
            TextChunk(
                id=chunk_id,
                book_slug=book_slug,
                title=meta.display_name,
                author=meta.author,
                year=meta.year,
                citation_key=meta.citation_key,
                page=page_num,
                chunk_index=idx,
                text=chunk_text,
            )
        )
        if end == len(words):
            break
        start += chunk_size - overlap
        idx += 1

    return chunks


def extract_chunks_from_document(doc_path: Path, meta: BookMetadata) -> list[TextChunk]:
    """Extract all text chunks from a PDF or EPUB document."""
    chunks: list[TextChunk] = []
    try:
        doc = pymupdf.open(doc_path)
    except Exception as exc:  # noqa: BLE001
        typer.secho(f"Warning: Failed to open {doc_path.name}: {exc}", fg=typer.colors.YELLOW)
        return []

    for page_idx in range(len(doc)):
        page_num = page_idx + 1
        page_text = str(doc[page_idx].get_text())
        page_chunks = extract_page_chunks(meta, page_num, page_text)
        chunks.extend(page_chunks)

    doc.close()
    return chunks


def get_embedding_model() -> TextEmbedding:
    """Initialize FastEmbed ONNX embedding model."""
    return TextEmbedding(model_name=EMBEDDING_MODEL_NAME)


def get_vector_db() -> lancedb.DBConnection:
    """Connect to the LanceDB database."""
    DB_PATH.mkdir(parents=True, exist_ok=True)
    return lancedb.connect(str(DB_PATH))


def has_table(db: lancedb.DBConnection, name: str) -> bool:
    """Check if table exists in LanceDB connection."""
    res = db.list_tables()
    if hasattr(res, "tables"):
        return name in res.tables
    return name in list(res)


def discover_book_files() -> list[Path]:
    """List all supported literature files in resources/ dynamically."""
    if not RESOURCES_DIR.is_dir():
        return []
    files: list[Path] = []
    for entry in sorted(RESOURCES_DIR.iterdir()):
        if entry.is_file() and entry.suffix.lower() in SUPPORTED_EXTENSIONS:
            files.append(entry)
    return files


def _filter_files(files: list[Path], book_filter: str | None) -> list[Path]:
    """Filter files by book slug or exit if none match."""
    if not book_filter:
        return files
    filtered = [f for f in files if book_filter.lower() in f.stem.lower()]
    if not filtered:
        typer.secho(f"No books matched filter '{book_filter}'.", fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)
    return filtered


def _process_single_book(
    file_path: Path, meta: BookMetadata, model: TextEmbedding
) -> list[dict[str, object]]:
    """Extract and embed all chunks for a single book."""
    typer.echo(f"Extracting chunks from: {file_path.name} ({meta.title})...")
    chunks = extract_chunks_from_document(file_path, meta)
    if not chunks:
        return []

    typer.echo(f"  Extracted {len(chunks)} chunks. Generating embeddings...")
    texts = [c.text for c in chunks]
    embeddings = list(model.embed(texts, batch_size=64))

    records: list[dict[str, object]] = []
    for chunk, emb in zip(chunks, embeddings, strict=True):
        records.append(
            {
                "id": chunk.id,
                "vector": emb.tolist(),
                "book_slug": chunk.book_slug,
                "title": chunk.title,
                "author": chunk.author,
                "year": chunk.year,
                "citation_key": chunk.citation_key,
                "page": chunk.page,
                "chunk_index": chunk.chunk_index,
                "text": chunk.text,
            }
        )
    return records


def _persist_records(
    db: lancedb.DBConnection,
    records: list[dict[str, object]],
    book_filter: str | None,
    target_slug: str | None,
) -> None:
    """Store or update records in LanceDB."""
    if not has_table(db, COLLECTION_NAME):
        db.create_table(COLLECTION_NAME, data=records, mode="overwrite")
        return

    if book_filter and target_slug:
        tbl = db.open_table(COLLECTION_NAME)
        tbl.delete(f"book_slug = '{target_slug}'")
        tbl.add(records)
    else:
        db.create_table(COLLECTION_NAME, data=records, mode="overwrite")


@app.command()
def index(
    book: Annotated[
        str | None,
        typer.Option("--book", "-b", help="Index only a specific book slug"),
    ] = None,
) -> None:
    """Index literature files into local LanceDB vector database."""
    files = _filter_files(discover_book_files(), book)
    typer.secho(f"Discovered {len(files)} literature files for indexing.", fg=typer.colors.CYAN)

    catalog = load_bib_metadata(BIB_PATH)
    model = get_embedding_model()
    db = get_vector_db()
    all_records: list[dict[str, object]] = []

    for file_path in files:
        meta = resolve_book_metadata(file_path, catalog)
        book_records = _process_single_book(file_path, meta, model)
        all_records.extend(book_records)

    if not all_records:
        typer.secho("No chunks extracted to index.", fg=typer.colors.YELLOW)
        return

    typer.secho(f"Persisting {len(all_records)} vectors into LanceDB...", fg=typer.colors.CYAN)
    target_slug = files[0].stem if book and files else None
    _persist_records(db, all_records, book, target_slug)

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
    """Perform semantic search across indexed literature."""
    db = get_vector_db()
    if not has_table(db, COLLECTION_NAME):
        typer.secho(
            "Vector database not found. Run 'python scripts/rag.py index' first.",
            fg=typer.colors.RED,
            err=True,
        )
        raise typer.Exit(code=1)

    table = db.open_table(COLLECTION_NAME)
    model = get_embedding_model()

    query_vec = list(model.embed([query]))[0].tolist()
    search_builder = table.search(query_vec).limit(limit)
    if book:
        search_builder = search_builder.where(f"book_slug LIKE '%{book}%'")

    results = search_builder.to_list()
    if not results:
        typer.secho(f"No results found for query: '{query}'", fg=typer.colors.YELLOW)
        return

    typer.secho(f"\nTop {len(results)} matches for: '{query}'\n", fg=typer.colors.CYAN, bold=True)
    for i, item in enumerate(results, start=1):
        score_val = item.get("_distance", 0.0)
        typer.secho(
            f"[{i}] {item['title']} (Page {item['page']}) — Distance: {score_val:.4f}",
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
