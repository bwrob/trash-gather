#!/usr/bin/env python3
"""Resources & Literature Parity Linter.

Validates bidirectional consistency between:
1. Physical literature files in resources/ (*.pdf, *.epub, *.djvu)
2. BibTeX entries in resources/references.bib
3. Catalog table and sections in resources/INDEX.md
4. Strict filename conventions (<author>_<year>_<slug>.<ext>)
"""

from __future__ import annotations

import re
from pathlib import Path

import typer

app = typer.Typer(
    add_completion=False,
    help="Validate parity across resources/, references.bib, and INDEX.md.",
)

MEDIA_EXTENSIONS = {".pdf", ".epub", ".djvu"}
FILENAME_PATTERN = re.compile(r"^[a-z0-9]+_[0-9]{4}_[a-z0-9_]+\.(pdf|epub|djvu)$")


def get_physical_media_files(resources_dir: Path) -> dict[str, Path]:
    """Collect all physical media files in resources/."""
    files: dict[str, Path] = {}
    for entry in resources_dir.iterdir():
        if entry.is_file() and entry.suffix.lower() in MEDIA_EXTENSIONS:
            files[entry.name] = entry
    return files


def parse_bib_file(bib_path: Path) -> tuple[dict[str, str], list[str]]:
    """Parse references.bib extracting file paths and citation keys."""
    bib_files: dict[str, str] = {}  # filename -> cite_key
    errors: list[str] = []

    if not bib_path.exists():
        errors.append(f"BibTeX file does not exist: {bib_path}")
        return bib_files, errors

    content = bib_path.read_text(encoding="utf-8")
    # Matches @type{key, ... file = {path}, ... }
    entries = re.split(r"(?=@\w+\s*\{)", content)

    for entry in entries:
        if not entry.strip():
            continue
        key_match = re.search(r"@\w+\s*\{\s*([^,\s]+)", entry)
        if not key_match:
            continue
        cite_key = key_match.group(1).strip()

        file_match = re.search(r"file\s*=\s*\{([^}]+)\}", entry)
        if not file_match:
            errors.append(f"BibTeX entry '@{cite_key}' is missing a 'file = {{...}}' field.")
            continue

        raw_path = file_match.group(1).strip()
        filename = Path(raw_path).name
        bib_files[filename] = cite_key

    return bib_files, errors


def parse_index_file(index_path: Path) -> tuple[set[str], list[str]]:
    """Parse INDEX.md extracting referenced media filenames."""
    index_files: set[str] = set()
    errors: list[str] = []

    if not index_path.exists():
        errors.append(f"Index file does not exist: {index_path}")
        return index_files, errors

    content = index_path.read_text(encoding="utf-8")

    # Match `- **File**: `resources/<filename>``
    file_matches = re.findall(r"-\s+\*\*File\*\*:\s*`([^`]+)`", content)
    for raw in file_matches:
        filename = Path(raw.split()[0]).name
        index_files.add(filename)

    # Match markdown links to resources in table or prose
    link_matches = re.findall(r"\[[^\]]+\]\(([^)]+)\)", content)
    for link in link_matches:
        link_target = link.strip().split("#")[0]
        if any(link_target.endswith(ext) for ext in MEDIA_EXTENSIONS):
            filename = Path(link_target).name
            index_files.add(filename)

    return index_files, errors


def check_naming_conventions(files: dict[str, Path]) -> list[str]:
    """Assert all media files follow <author>_<year>_<slug>.<ext>."""
    errors: list[str] = []
    for name in sorted(files.keys()):
        if not FILENAME_PATTERN.match(name):
            errors.append(
                f"File '{name}' violates naming convention '<author>_<year>_<slug>.<ext>'."
            )
    return errors


def check_parity(
    physical_files: dict[str, Path],
    bib_files: dict[str, str],
    index_files: set[str],
) -> list[str]:
    """Assert bidirectional parity between physical files, bib, and index."""
    errors: list[str] = []
    phys_set = set(physical_files.keys())
    bib_set = set(bib_files.keys())

    # 1. Physical vs BibTeX
    untracked_in_bib = sorted(phys_set - bib_set)
    for f in untracked_in_bib:
        errors.append(f"Physical file 'resources/{f}' has no entry in references.bib.")

    missing_bib_files = sorted(bib_set - phys_set)
    for f in missing_bib_files:
        errors.append(f"references.bib refers to 'resources/{f}', but file is missing on disk.")

    # 2. Physical vs INDEX.md
    unindexed = sorted(phys_set - index_files)
    for f in unindexed:
        errors.append(f"Physical file 'resources/{f}' is not indexed in resources/INDEX.md.")

    missing_index_files = sorted(index_files - phys_set)
    for f in missing_index_files:
        errors.append(f"resources/INDEX.md refers to '{f}', but file is missing on disk.")

    return errors


@app.command()
def main() -> None:
    """Validate parity across resources/, references.bib, and INDEX.md."""
    repo_root = Path(__file__).resolve().parent.parent
    resources_dir = repo_root / "resources"
    bib_path = resources_dir / "references.bib"
    index_path = resources_dir / "INDEX.md"

    if not resources_dir.is_dir():
        typer.secho("ERROR: resources/ directory not found.", fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)

    physical_files = get_physical_media_files(resources_dir)
    bib_files, bib_errors = parse_bib_file(bib_path)
    index_files, index_errors = parse_index_file(index_path)

    all_errors: list[str] = []
    all_errors.extend(bib_errors)
    all_errors.extend(index_errors)
    all_errors.extend(check_naming_conventions(physical_files))
    all_errors.extend(check_parity(physical_files, bib_files, index_files))

    if all_errors:
        typer.secho(
            f"FAILED: Found {len(all_errors)} resource parity error(s):\n",
            fg=typer.colors.RED,
            bold=True,
            err=True,
        )
        for err in all_errors:
            typer.secho(f"  ❌ {err}", fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)

    typer.secho(
        f"✅ Resource parity clean: {len(physical_files)} media volumes fully synchronized "
        "across resources/, references.bib, and INDEX.md.",
        fg=typer.colors.GREEN,
        bold=True,
    )


if __name__ == "__main__":
    app()
