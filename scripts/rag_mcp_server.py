#!/usr/bin/env python3
"""Model Context Protocol (MCP) Server for Literature RAG.

Exposes local semantic search and page extraction tools over standard I/O (stdio)
for Antigravity agents paired on systems programming and memory management.
Dynamically resolves book metadata from resources/references.bib.
"""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

import lancedb
import pymupdf
from fastembed import TextEmbedding
from mcp.server.fastmcp import FastMCP

_scripts_dir = str(Path(__file__).resolve().parent)
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)

from rag import (  # noqa: E402
    BGE_QUERY_PREFIX,
    COLLECTION_NAME,
    DB_PATH,
    EMBEDDING_MODEL_NAME,
    RESOURCES_DIR,
    _execute_search,
    has_table,
    load_bib_metadata,
    resolve_book_metadata,
)

mcp = FastMCP("literature-rag")

BIB_PATH: Path = RESOURCES_DIR / "references.bib"

# Lazy-loaded globals to avoid startup lag
_db_client: lancedb.DBConnection | None = None
_embedding_model: TextEmbedding | None = None


def get_db() -> lancedb.DBConnection:
    """Return cached LanceDB client."""
    global _db_client
    if _db_client is None:
        DB_PATH.mkdir(parents=True, exist_ok=True)
        _db_client = lancedb.connect(str(DB_PATH))
    return _db_client


def get_model() -> TextEmbedding:
    """Return cached FastEmbed model."""
    global _embedding_model
    if _embedding_model is None:
        _embedding_model = TextEmbedding(model_name=EMBEDDING_MODEL_NAME)
    return _embedding_model


@mcp.tool()
def search_literature(query: str, book: str = "", limit: int = 5) -> str:
    """Semantically search systems programming, C, and Garbage Collection literature.

    Args:
        query: Conceptual search string (e.g. 'mark sweep phase', 'pointer aliasing rules').
        book: Optional book slug or keyword filter (e.g. 'jones', 'seacord', 'bryant', 'nystrom').
        limit: Maximum number of relevant excerpts to return (default 5).

    Returns:
        Markdown-formatted list of matching passages with book titles and page numbers.
    """
    db = get_db()
    if not has_table(db, COLLECTION_NAME):
        return (
            "⚠️ Vector database is not initialized yet. "
            "Please run 'just rag-index' in the terminal to index the literature."
        )

    table = db.open_table(COLLECTION_NAME)
    model = get_model()

    prefixed_query = f"{BGE_QUERY_PREFIX}{query}"
    query_vec = list(model.embed([prefixed_query]))[0].tolist()
    filter_book = book.strip().lower() if book.strip() else None
    results: list[dict[str, Any]] = _execute_search(table, query, query_vec, filter_book, limit)
    if not results:
        return f"No relevant literature found for query: '{query}'."

    output_lines: list[str] = [f"### Literature Search Results for: *{query}*\n"]
    for i, item in enumerate(results, start=1):
        title = item.get("title", item.get("book_slug", "Unknown Book"))
        page_start = item.get("page", "?")
        page_end = item.get("page_end")
        page_str = (
            f"{page_start}–{page_end}" if page_end and page_end != page_start else f"{page_start}"
        )
        section_str = f" | {item['section']}" if item.get("section") else ""
        rel = item.get("_relevance_score")
        dist = item.get("_distance", 0.0)
        dist_f = float(str(dist)) if dist is not None else 0.0
        score = float(str(rel)) if rel is not None else (1.0 - dist_f)
        text = str(item.get("text", "")).strip()

        output_lines.append(
            f"**[{i}] {title} — Page {page_str}{section_str}** (Score: {score:.3f})"
        )
        output_lines.append(f"> {text}\n")

    return "\n".join(output_lines)


@mcp.tool()
def get_book_page(book_slug: str, page_number: int) -> str:
    """Retrieve the full text of a specific page from a textbook in resources/.

    Args:
        book_slug: Substring matching the book filename (e.g. 'jones', 'nystrom', 'seacord').
        page_number: 1-indexed page number.

    Returns:
        The extracted plain text of that page.
    """
    target_file: Path | None = None
    for entry in RESOURCES_DIR.iterdir():
        if entry.is_file() and book_slug.lower() in entry.name.lower():
            target_file = entry
            break

    if target_file is None:
        return f"Error: No literature file found matching slug '{book_slug}' in resources/."

    try:
        doc = pymupdf.open(target_file)
        if page_number < 1 or page_number > len(doc):
            return f"Error: Page {page_number} is out of bounds (book has {len(doc)} pages)."
        page_text = str(doc[page_number - 1].get_text())
        doc.close()
        if not page_text.strip():
            return f"[Page {page_number} of {target_file.name} appears to be empty or image-only]"
        return f"### {target_file.name} (Page {page_number})\n\n{page_text}"
    except Exception as exc:  # noqa: BLE001
        return f"Error reading page {page_number} from {target_file.name}: {exc}"


@mcp.tool()
def list_literature_books() -> str:
    """List all available literature books, page counts, and indexing status.

    Returns:
        Markdown table of available textbooks.
    """
    if not RESOURCES_DIR.is_dir():
        return "resources/ directory does not exist."

    db = get_db()
    indexed = has_table(db, COLLECTION_NAME)
    catalog = load_bib_metadata(BIB_PATH)

    lines: list[str] = [
        "| Book Title | File | Pages | Format |",
        "| :--- | :--- | :--- | :--- |",
    ]

    for entry in sorted(RESOURCES_DIR.iterdir()):
        if entry.is_file() and entry.suffix.lower() in {".pdf", ".epub", ".djvu"}:
            meta = resolve_book_metadata(entry, catalog)
            try:
                doc = pymupdf.open(entry)
                pages = str(len(doc))
                doc.close()
            except Exception:  # noqa: BLE001
                pages = "N/A"
            lines.append(f"| **{meta.display_name}** | `{entry.name}` | {pages} | {entry.suffix} |")

    status_note = "✅ Indexed in LanceDB" if indexed else "⚠️ Not yet indexed (run 'just rag-index')"
    return f"**Literature Catalog Status**: {status_note}\n\n" + "\n".join(lines)


if __name__ == "__main__":
    mcp.run()
