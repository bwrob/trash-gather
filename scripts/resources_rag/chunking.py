"""Layout-aware and semantic document chunking engine."""

from __future__ import annotations

import bisect
import re
from dataclasses import dataclass
from pathlib import Path

import pymupdf
from models import BookMetadata, TextChunk

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


@dataclass
class DocBlock:
    """Internal representation of an extracted document block."""

    page: int
    section: str
    text: str
    is_atomic: bool = False


def is_margin_noise(text: str, y0: float, y1: float, page_height: float) -> bool:
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


def clean_block_text(text: str) -> tuple[str, bool]:
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


def build_toc_lookup(doc: pymupdf.Document) -> tuple[list[int], list[str]]:
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


def extract_page_blocks(page: pymupdf.Page, page_num: int, sec: str) -> list[DocBlock]:
    """Extract and filter valid text/code blocks from a single page."""
    p_height = page.rect.height
    page_blocks: list[DocBlock] = []

    for b in page.get_text("blocks"):
        if b[6] != 0:
            continue
        raw_text = b[4].strip()
        if not raw_text or is_margin_noise(raw_text, b[1], b[3], p_height):
            continue
        cleaned, is_atomic = clean_block_text(raw_text)
        if cleaned:
            page_blocks.append(
                DocBlock(page=page_num, section=sec, text=cleaned, is_atomic=is_atomic)
            )

    return page_blocks


def extract_document_blocks(doc: pymupdf.Document) -> list[DocBlock]:
    """Extract filtered, cleaned text and code blocks from all pages in document."""
    toc_pages, toc_titles = build_toc_lookup(doc)

    def get_section(page_num: int) -> str:
        if not toc_pages:
            return ""
        idx = bisect.bisect_right(toc_pages, page_num) - 1
        return toc_titles[idx] if idx >= 0 else ""

    blocks: list[DocBlock] = []
    for page_idx in range(len(doc)):
        page_num = page_idx + 1
        sec = get_section(page_num)
        blocks.extend(extract_page_blocks(doc[page_idx], page_num, sec))

    return blocks


def split_oversized_block(
    b: DocBlock,
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


def make_chunk_from_blocks(
    blist: list[DocBlock],
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


def compute_overlap_blocks(
    blocks: list[DocBlock],
    overlap_words: int,
) -> tuple[list[DocBlock], int]:
    """Extract trailing blocks that satisfy the target overlap word count."""
    overlap_blocks: list[DocBlock] = []
    overlap_count = 0
    for ob in reversed(blocks):
        ob_w = len(ob.text.split())
        overlap_blocks.insert(0, ob)
        overlap_count += ob_w
        if overlap_count >= overlap_words:
            break
    return overlap_blocks, overlap_count


def should_flush_chunk(
    curr_blocks: list[DocBlock],
    curr_words: int,
    b: DocBlock,
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


def assemble_text_chunks(
    blocks: list[DocBlock],
    meta: BookMetadata,
    max_words: int = 240,
    overlap_words: int = 40,
) -> list[TextChunk]:
    """Group document blocks into chunks respecting word limits, overlap, and boundaries."""
    book_slug = Path(meta.filename).stem
    chunks: list[TextChunk] = []
    curr_blocks: list[DocBlock] = []
    curr_words = 0
    chunk_idx = 0

    for b in blocks:
        b_w = len(b.text.split())
        if not b_w:
            continue

        if b_w > max_words and not b.is_atomic:
            if curr_blocks:
                chunks.append(make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))
                chunk_idx += 1
                curr_blocks = []
                curr_words = 0
            sub_chunks, chunk_idx = split_oversized_block(
                b, meta, book_slug, max_words, overlap_words, chunk_idx
            )
            chunks.extend(sub_chunks)
            continue

        if should_flush_chunk(curr_blocks, curr_words, b, b_w, max_words):
            chunks.append(make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))
            chunk_idx += 1
            curr_blocks, curr_words = compute_overlap_blocks(curr_blocks, overlap_words)

        curr_blocks.append(b)
        curr_words += b_w

    if curr_blocks:
        chunks.append(make_chunk_from_blocks(curr_blocks, meta, book_slug, chunk_idx))

    return chunks


def extract_chunks_from_document(doc_path: Path, meta: BookMetadata) -> list[TextChunk]:
    """Extract structured, layout-aware text chunks across document pages."""
    try:
        doc = pymupdf.open(doc_path)
    except Exception:  # noqa: BLE001
        return []

    blocks = extract_document_blocks(doc)
    doc.close()
    return assemble_text_chunks(blocks, meta)
