"""Domain models, configurations, and metadata loaders for literature RAG."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent.parent
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


def clean_bib_val(val: str | None) -> str:
    """Clean extra braces and normalize whitespace from BibTeX field values."""
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
        title=clean_bib_val(title_m.group(1) if title_m else fn),
        author=clean_bib_val(author_m.group(1) if author_m else ""),
        year=clean_bib_val(year_m.group(1) if year_m else ""),
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


def discover_book_files() -> list[Path]:
    """List all supported literature files in resources/ dynamically."""
    if not RESOURCES_DIR.is_dir():
        return []
    files: list[Path] = []
    for entry in sorted(RESOURCES_DIR.iterdir()):
        if entry.is_file() and entry.suffix.lower() in SUPPORTED_EXTENSIONS:
            files.append(entry)
    return files
