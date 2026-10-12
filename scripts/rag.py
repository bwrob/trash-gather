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
from lancedb.index import FTS
from lancedb.rerankers import LinearCombinationReranker
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

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent
RESOURCES_DIR: Path = WORKSPACE_ROOT / "resources"
BIB_PATH: Path = RESOURCES_DIR / "references.bib"
DB_PATH: Path = WORKSPACE_ROOT / ".rag_db"
COLLECTION_NAME: str = "literature"
EMBEDDING_MODEL_NAME: str = "BAAI/bge-small-en-v1.5"
BGE_QUERY_PREFIX: str = "Represent this sentence for searching relevant passages: "
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
    section: str = ""
    page_end: int = 0


@dataclass
class _DocBlock:
    """Internal representation of an extracted document block."""

    page: int
    section: str
    text: str
    is_atomic: bool = False


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


def _is_margin_noise(text: str, y0: float, y1: float, page_height: float) -> bool:
    """Detect repetitive running headers, footers, or standalone page numbers."""
    t = text.strip()
    if not t:
        return True
    if y0 < 45.0 or y1 > page_height - 45.0:
        if t.isdigit() or re.fullmatch(r"p(age)?\.?\s*\d+", t, re.IGNORECASE):
            return True
        if len(t) <= 60 and len(t.splitlines()) <= 2:
            return True
    return False


MATH_SYMBOLS: set[str] = set("∀∃∈∉⊂⊆∪∩∧∨¬⇒⇔∑∏∫≤≥≠≈≡→←↑↓")
CODE_INDICATORS: tuple[str, ...] = (
    "  ",
    "\t",
    "#include",
    "#define",
    "#ifdef",
    "struct ",
    "typedef ",
    "int ",
    "void ",
    "char *",
    "return ",
    "if (",
    "for (",
    "while (",
    "switch (",
)


def _clean_block_text(text: str) -> tuple[str, bool]:
    """Clean text while preserving code block and math layout without corruption."""
    text = text.replace("\xad", "")
    lines = text.strip().splitlines()
    if not lines:
        return "", False

    is_code = any(line.startswith(CODE_INDICATORS) for line in lines) and len(lines) > 1
    if is_code:
        return "\n".join(line.rstrip() for line in lines if line.strip()), True

    is_math = any(char in MATH_SYMBOLS for char in text)
    if is_math and len(lines) > 1:
        return "\n".join(line.rstrip() for line in lines if line.strip()), True

    healed = re.sub(r"(\w+)-\n(\w+)", r"\1\2", text)
    return re.sub(r"\s+", " ", healed).strip(), False


def _build_toc_lookup(doc: pymupdf.Document) -> tuple[list[int], list[str]]:
    """Extract sorted table of contents page numbers and section titles."""
    raw_toc = doc.get_toc()
    toc_pages: list[int] = []
    toc_titles: list[str] = []
    for item in raw_toc:
        if len(item) >= 3 and isinstance(item[2], int) and item[2] > 0:
            toc_pages.append(item[2])
            toc_titles.append(item[1].strip())

    if not toc_pages:
        return [], []

    def _sort_key(pair: tuple[int, str]) -> int:
        return pair[0]

    sorted_pairs = sorted(zip(toc_pages, toc_titles, strict=True), key=_sort_key)
    return [p for p, _ in sorted_pairs], [t for _, t in sorted_pairs]


def _extract_page_blocks(page: pymupdf.Page, page_num: int, sec: str) -> list[_DocBlock]:
    """Extract and filter valid text/code blocks from a single page."""
    p_height = page.rect.height
    page_blocks: list[_DocBlock] = []

    for b in page.get_text("blocks"):
        if b[6] != 0:
            continue
        raw_text = b[4].strip()
        if not raw_text or _is_margin_noise(raw_text, b[1], b[3], p_height):
            continue
        cleaned, is_atomic = _clean_block_text(raw_text)
        if cleaned:
            page_blocks.append(
                _DocBlock(page=page_num, section=sec, text=cleaned, is_atomic=is_atomic)
            )

    return page_blocks


def _extract_document_blocks(doc: pymupdf.Document) -> list[_DocBlock]:
    """Extract filtered, cleaned text and code blocks from all pages in document."""
    import bisect

    toc_pages, toc_titles = _build_toc_lookup(doc)

    def get_section(page_num: int) -> str:
        if not toc_pages:
            return ""
        idx = bisect.bisect_right(toc_pages, page_num) - 1
        return toc_titles[idx] if idx >= 0 else ""

    blocks: list[_DocBlock] = []
    for page_idx in range(len(doc)):
        page_num = page_idx + 1
        sec = get_section(page_num)
        blocks.extend(_extract_page_blocks(doc[page_idx], page_num, sec))

    return blocks


def _split_oversized_block(
    b: _DocBlock,
    meta: BookMetadata,
    book_slug: str,
    max_words: int,
    overlap_words: int,
    start_chunk_idx: int,
) -> tuple[list[TextChunk], int]:
    """Split a single block whose word count exceeds max_words."""
    words = b.text.split()
    chunks: list[TextChunk] = []
    chunk_idx = start_chunk_idx
    st = 0

    while st < len(words):
        end = min(st + max_words, len(words))
        sub_text = " ".join(words[st:end])
        cid = f"{book_slug}_p{b.page}_{chunk_idx}"
        chunks.append(
            TextChunk(
                id=cid,
                book_slug=book_slug,
                title=meta.display_name,
                author=meta.author,
                year=meta.year,
                citation_key=meta.citation_key,
                page=b.page,
                chunk_index=chunk_idx,
                text=sub_text,
                section=b.section,
                page_end=b.page,
            )
        )
        chunk_idx += 1
        if end == len(words):
            break
        st += max_words - overlap_words

    return chunks, chunk_idx


def _make_chunk_from_blocks(
    blist: list[_DocBlock],
    meta: BookMetadata,
    book_slug: str,
    chunk_idx: int,
) -> TextChunk:
    """Construct a TextChunk from an accumulated sequence of document blocks."""
    p_start = blist[0].page
    p_end = blist[-1].page
    sec = blist[0].section or blist[-1].section
    body = "\n\n".join(b.text for b in blist)
    cid = f"{book_slug}_p{p_start}_{chunk_idx}"
    return TextChunk(
        id=cid,
        book_slug=book_slug,
        title=meta.display_name,
        author=meta.author,
        year=meta.year,
        citation_key=meta.citation_key,
        page=p_start,
        chunk_index=chunk_idx,
        text=body,
        section=sec,
        page_end=p_end,
    )


def _compute_overlap_blocks(
    blocks: list[_DocBlock],
    overlap_words: int,
) -> tuple[list[_DocBlock], int]:
    """Extract trailing blocks that satisfy the target overlap word count."""
    overlap_blocks: list[_DocBlock] = []
    overlap_count = 0
    for ob in reversed(blocks):
        ob_w = len(ob.text.split())
        overlap_blocks.insert(0, ob)
        overlap_count += ob_w
        if overlap_count >= overlap_words:
            break
    return overlap_blocks, overlap_count


def _should_flush_chunk(
    curr_blocks: list[_DocBlock],
    curr_words: int,
    b: _DocBlock,
    b_words: int,
    max_words: int,
) -> bool:
    """Determine whether the current accumulator should be finalized as a chunk."""
    if not curr_blocks:
        return False
    # Semantic heading boundary: split if section changes between blocks
    if b.section and curr_blocks[0].section and b.section != curr_blocks[0].section:
        return True
    # Word budget boundary exceeded
    return curr_words + b_words > max_words


def _assemble_text_chunks(
    blocks: list[_DocBlock],
    meta: BookMetadata,
    max_words: int = 240,
    overlap_words: int = 40,
) -> list[TextChunk]:
    """Group document blocks into chunks respecting word limits, overlap, and boundaries."""
    book_slug = Path(meta.filename).stem
    chunks: list[TextChunk] = []
    curr_blocks: list[_DocBlock] = []
    curr_words = 0
    chunk_idx = 0

    for b in blocks:
        b_w = len(b.text.split())
        if not b_w:
            continue

        if b_w > max_words and not b.is_atomic:
            if curr_blocks:
                chunks.append(_make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))
                chunk_idx += 1
                curr_blocks = []
                curr_words = 0
            sub_chunks, chunk_idx = _split_oversized_block(
                b, meta, book_slug, max_words, overlap_words, chunk_idx
            )
            chunks.extend(sub_chunks)
            continue

        if _should_flush_chunk(curr_blocks, curr_words, b, b_w, max_words):
            chunks.append(_make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))
            chunk_idx += 1
            curr_blocks, curr_words = _compute_overlap_blocks(curr_blocks, overlap_words)

        curr_blocks.append(b)
        curr_words += b_w

    if curr_blocks:
        chunks.append(_make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))

    return chunks


def extract_chunks_from_document(doc_path: Path, meta: BookMetadata) -> list[TextChunk]:
    """Extract structured, layout-aware text chunks across document pages."""
    try:
        doc = pymupdf.open(doc_path)
    except Exception as exc:  # noqa: BLE001
        typer.secho(f"Warning: Failed to open {doc_path.name}: {exc}", fg=typer.colors.YELLOW)
        return []

    blocks = _extract_document_blocks(doc)
    doc.close()
    return _assemble_text_chunks(blocks, meta)


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


def _build_chunk_record(chunk: TextChunk, emb: object) -> dict[str, object]:
    """Build a database record dictionary from a text chunk and vector."""
    return {
        "id": chunk.id,
        "vector": emb.tolist(),  # type: ignore[union-attr]
        "book_slug": chunk.book_slug,
        "title": chunk.title,
        "author": chunk.author,
        "year": chunk.year,
        "citation_key": chunk.citation_key,
        "page": chunk.page,
        "page_end": chunk.page_end,
        "section": chunk.section,
        "chunk_index": chunk.chunk_index,
        "text": chunk.text,
    }


def _collect_embeddings(
    model: TextEmbedding,
    texts: list[str],
    progress: Progress | None,
    chunk_task: TaskID | None,
) -> list[object]:
    """Generate vector embeddings, optionally updating progress."""
    embeddings: list[object] = []
    for emb in model.embed(texts, batch_size=64):
        embeddings.append(emb)
        if progress and chunk_task is not None:
            progress.advance(chunk_task)
    return embeddings


def _process_single_book(
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

    embeddings = _collect_embeddings(model, embed_texts, progress, chunk_task)
    return [_build_chunk_record(c, e) for c, e in zip(chunks, embeddings, strict=True)]


def _upgrade_and_merge_records(
    db: lancedb.DBConnection,
    tbl: lancedb.table.Table,
    records: list[dict[str, object]],
    target_slug: str,
) -> None:
    """Migrate legacy LanceDB table schema and merge newly indexed records."""
    import pyarrow as pa

    existing_arrow = tbl.to_arrow()
    if "section" not in existing_arrow.schema.names:
        n = len(existing_arrow)
        existing_arrow = existing_arrow.append_column("section", pa.array([""] * n, pa.string()))
    if "page_end" not in existing_arrow.schema.names:
        existing_arrow = existing_arrow.append_column("page_end", existing_arrow["page"])

    mask = [str(slug) != target_slug for slug in existing_arrow["book_slug"]]
    filtered_existing = existing_arrow.filter(mask)
    new_table = pa.Table.from_pylist(records, schema=filtered_existing.schema)
    combined = pa.concat_tables([filtered_existing, new_table])
    db.create_table(COLLECTION_NAME, data=combined, mode="overwrite")


def _ensure_fts_index(tbl: lancedb.table.Table) -> None:
    """Ensure Full-Text Search (FTS) index is built on the text column."""
    try:
        tbl.create_index("text", config=FTS(), replace=True)
    except Exception as exc:  # noqa: BLE001
        typer.secho(f"Warning: Failed to create FTS index: {exc}", fg=typer.colors.YELLOW)


def _persist_records(
    db: lancedb.DBConnection,
    records: list[dict[str, object]],
    book_filter: str | None,
    target_slug: str | None,
) -> None:
    """Store or update records in LanceDB, ensuring schema compatibility and FTS."""
    if not has_table(db, COLLECTION_NAME):
        tbl = db.create_table(COLLECTION_NAME, data=records, mode="overwrite")
        _ensure_fts_index(tbl)
        return

    if not (book_filter and target_slug):
        tbl = db.create_table(COLLECTION_NAME, data=records, mode="overwrite")
        _ensure_fts_index(tbl)
        return

    tbl = db.open_table(COLLECTION_NAME)
    field_names = [f.name for f in tbl.schema]
    if "section" in field_names and "page_end" in field_names:
        tbl.delete(f"book_slug = '{target_slug}'")
        tbl.add(records)
    else:
        _upgrade_and_merge_records(db, tbl, records, target_slug)
        tbl = db.open_table(COLLECTION_NAME)
    _ensure_fts_index(tbl)


def _execute_search(
    table: lancedb.table.Table,
    query: str,
    query_vec: list[float],
    book: str | None,
    limit: int,
) -> list[dict[str, object]]:
    """Execute hybrid search if FTS index exists, falling back to pure vector search."""
    has_fts = any(
        getattr(idx, "index_type", "") == "FTS" or "text" in getattr(idx, "columns", [])
        for idx in table.list_indices()
    )

    if has_fts:
        builder = (
            table.search(query_type="hybrid")
            .vector(query_vec)
            .text(query)
            .rerank(reranker=LinearCombinationReranker(weight=0.7))
            .limit(limit)
        )
    else:
        builder = table.search(query_vec).limit(limit)

    if book:
        builder = builder.where(f"book_slug LIKE '%{book}%'")

    return builder.to_list()


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
            book_records = _process_single_book(
                file_path, meta, model, progress=progress, chunk_task=chunk_task
            )
            all_records.extend(book_records)
            progress.advance(book_task)

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
    """Perform hybrid semantic & full-text search across indexed literature."""
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

    # BGE query instruction prefix for asymmetric retrieval
    prefixed_query = f"{BGE_QUERY_PREFIX}{query}"
    query_vec = list(model.embed([prefixed_query]))[0].tolist()
    results = _execute_search(table, query, query_vec, book, limit)

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
