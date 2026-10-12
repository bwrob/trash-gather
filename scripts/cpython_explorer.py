#!/usr/bin/env python3
"""CPython Runtime Reference & Source Explorer.

Fetches and caches canonical CPython internal runtime files (Objects, Include,
Modules/gcmodule.c) to inspect architectural patterns for object headers,
immortal singletons, and sequence protocols.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated

import requests
import typer

app = typer.Typer(
    add_completion=False,
    help="Fetch, cache, and search canonical CPython runtime sources.",
)

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent
CACHE_DIR: Path = WORKSPACE_ROOT / ".cache" / "cpython"
CPYTHON_RAW_BASE: str = "https://raw.githubusercontent.com/python/cpython/main"

CANONICAL_CPYTHON_FILES: dict[str, str] = {
    "tuple": "Objects/tupleobject.c",
    "list": "Objects/listobject.c",
    "object": "Objects/object.c",
    "gc": "Modules/gcmodule.c",
    "obmalloc": "Objects/obmalloc.c",
    "object_h": "Include/object.h",
    "cpython_object_h": "Include/cpython/object.h",
    "tuple_h": "Include/tupleobject.h",
    "list_h": "Include/listobject.h",
}


def fetch_cpython_file(relative_path: str) -> tuple[str, bool]:
    """Fetch CPython file from local cache or download from GitHub."""
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    cache_path = CACHE_DIR / relative_path

    if cache_path.is_file():
        return cache_path.read_text(encoding="utf-8", errors="replace"), True

    url = f"{CPYTHON_RAW_BASE}/{relative_path}"
    try:
        resp = requests.get(url, timeout=10)
        resp.raise_for_status()
        content = resp.text
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        cache_path.write_text(content, encoding="utf-8")
        return content, False
    except Exception as exc:  # noqa: BLE001
        return f"Error fetching {relative_path} from CPython repository: {exc}", False


@app.command()
def view(
    path_or_slug: Annotated[
        str,
        typer.Argument(help="CPython relative path (or alias: tuple, list, gc, object)"),
    ],
    start_line: Annotated[int, typer.Option("--start", "-s", help="Start line (1-indexed)")] = 1,
    end_line: Annotated[int, typer.Option("--end", "-e", help="End line (1-indexed)")] = 100,
) -> None:
    """View a range of lines from a CPython source file."""
    rel_path = CANONICAL_CPYTHON_FILES.get(path_or_slug, path_or_slug)
    content, cached = fetch_cpython_file(rel_path)

    if content.startswith("Error fetching"):
        typer.secho(content, fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)

    lines = content.splitlines()
    total = len(lines)
    s = max(1, start_line)
    e = min(total, end_line)

    status_tag = "[cached]" if cached else "[downloaded]"
    header = f"\nCPython: {rel_path} (Lines {s}–{e} of {total}) {status_tag}\n"
    typer.secho(header, fg=typer.colors.CYAN, bold=True)

    for i in range(s - 1, e):
        line_num = i + 1
        typer.echo(f"{line_num:5d} | {lines[i]}")
    typer.echo()


@app.command()
def search(
    query: Annotated[str, typer.Argument(help="Regex pattern or keyword to search for")],
    path_or_slug: Annotated[
        str,
        typer.Option("--file", "-f", help="Target file (defaults to 'tuple')"),
    ] = "tuple",
    context: Annotated[
        int,
        typer.Option("--context", "-C", help="Lines of context around matches"),
    ] = 2,
) -> None:
    """Search for a keyword in a CPython source file."""
    rel_path = CANONICAL_CPYTHON_FILES.get(path_or_slug, path_or_slug)
    content, _ = fetch_cpython_file(rel_path)

    if content.startswith("Error fetching"):
        typer.secho(content, fg=typer.colors.RED, err=True)
        raise typer.Exit(code=1)

    lines = content.splitlines()
    regex = re.compile(query, re.IGNORECASE)
    matched_indices = [idx for idx, ln in enumerate(lines) if regex.search(ln)]

    if not matched_indices:
        typer.secho(f"No matches for '{query}' in {rel_path}.", fg=typer.colors.YELLOW)
        return

    msg = f"Found {len(matched_indices)} match(es) in {rel_path}:\n"
    typer.secho(msg, fg=typer.colors.CYAN, bold=True)
    shown_lines: set[int] = set()

    for idx in matched_indices[:10]:
        start = max(0, idx - context)
        end = min(len(lines), idx + context + 1)
        for cur in range(start, end):
            if cur not in shown_lines:
                prefix = ">" if cur == idx else " "
                typer.echo(f"{prefix} {cur + 1:5d} | {lines[cur]}")
                shown_lines.add(cur)
        typer.echo("---")


@app.command()
def list_canonical() -> None:
    """List canonical CPython source aliases available for exploration."""
    typer.secho("Canonical CPython Runtime Files:\n", fg=typer.colors.CYAN, bold=True)
    for alias, rel_path in sorted(CANONICAL_CPYTHON_FILES.items()):
        typer.echo(f"  • {alias:<18} -> {rel_path}")


if __name__ == "__main__":
    app()
