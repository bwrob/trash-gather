"""Vector embedding, hybrid retrieval, and database persistence engine."""

from __future__ import annotations

import contextlib

import lancedb
from fastembed import TextEmbedding
from lancedb.index import FTS
from lancedb.rerankers import LinearCombinationReranker
from models import COLLECTION_NAME, DB_PATH, EMBEDDING_MODEL_NAME, TextChunk
from rich.progress import Progress, TaskID


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


def build_chunk_record(chunk: TextChunk, emb: object) -> dict[str, object]:
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


def collect_embeddings(
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


def upgrade_and_merge_records(
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


def ensure_fts_index(tbl: lancedb.table.Table) -> None:
    """Ensure Full-Text Search (FTS) index is built on the text column."""
    with contextlib.suppress(Exception):
        tbl.create_index("text", config=FTS(), replace=True)


def persist_records(
    db: lancedb.DBConnection,
    records: list[dict[str, object]],
    book_filter: str | None,
    target_slug: str | None,
) -> None:
    """Store or update records in LanceDB, ensuring schema compatibility and FTS."""
    if not has_table(db, COLLECTION_NAME):
        tbl = db.create_table(COLLECTION_NAME, data=records, mode="overwrite")
        ensure_fts_index(tbl)
        return

    if not (book_filter and target_slug):
        tbl = db.create_table(COLLECTION_NAME, data=records, mode="overwrite")
        ensure_fts_index(tbl)
        return

    tbl = db.open_table(COLLECTION_NAME)
    field_names = [f.name for f in tbl.schema]
    if "section" in field_names and "page_end" in field_names:
        tbl.delete(f"book_slug = '{target_slug}'")
        tbl.add(records)
    else:
        upgrade_and_merge_records(db, tbl, records, target_slug)
        tbl = db.open_table(COLLECTION_NAME)
    ensure_fts_index(tbl)


def execute_search(
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
