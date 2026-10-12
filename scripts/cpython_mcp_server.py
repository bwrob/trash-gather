#!/usr/bin/env python3
"""Model Context Protocol (MCP) Server for CPython Runtime Reference.

Exposes canonical CPython internal source code fetching, viewing, and searching tools
over stdio to explore CPython's object layouts, immortal singletons, and sequence allocators.
"""

from __future__ import annotations

import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

_scripts_dir = str(Path(__file__).resolve().parent)
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)

from cpython_explorer import (  # noqa: E402
    CANONICAL_CPYTHON_FILES,
    fetch_cpython_file,
)

mcp = FastMCP("cpython-reference-explorer")


@mcp.tool()
def get_cpython_source(
    path_or_alias: str,
    start_line: int = 1,
    end_line: int = 80,
) -> str:
    """Fetch and view a range of lines from a canonical CPython source file.

    Args:
        path_or_alias: Relative path (e.g. 'Objects/tupleobject.c') or shortcut alias
            ('tuple', 'list', 'gc', 'object', 'obmalloc').
        start_line: 1-indexed starting line number.
        end_line: 1-indexed ending line number.

    Returns:
        Code slice with line numbers and file origin.
    """
    rel_path = CANONICAL_CPYTHON_FILES.get(path_or_alias, path_or_alias)
    content, cached = fetch_cpython_file(rel_path)

    if content.startswith("Error fetching"):
        return content

    lines = content.splitlines()
    total = len(lines)
    s = max(1, start_line)
    e = min(total, end_line)

    status_tag = "cached" if cached else "downloaded"
    output_lines: list[str] = [
        f"### CPython `{rel_path}` (Lines {s}–{e} of {total}, {status_tag})\n",
        "```c",
    ]
    for i in range(s - 1, e):
        output_lines.append(f"{i + 1:5d} | {lines[i]}")
    output_lines.append("```")

    return "\n".join(output_lines)


@mcp.tool()
def search_cpython_source(
    query: str,
    path_or_alias: str = "tuple",
    max_matches: int = 8,
) -> str:
    """Search for a keyword or symbol in a canonical CPython source file.

    Args:
        query: Keyword or symbol to search for (e.g. 'PyTuple_New', 'immortal', 'GC_UNTRACK').
        path_or_alias: Target file path or alias ('tuple', 'list', 'gc', 'object', 'obmalloc').
        max_matches: Maximum number of match occurrences to display.

    Returns:
        Formatted excerpts showing matching lines and surrounding context.
    """
    rel_path = CANONICAL_CPYTHON_FILES.get(path_or_alias, path_or_alias)
    content, _ = fetch_cpython_file(rel_path)

    if content.startswith("Error fetching"):
        return content

    lines = content.splitlines()
    matches = [i for i, line in enumerate(lines) if query.lower() in line.lower()]

    if not matches:
        return f"No matches found for '{query}' in CPython `{rel_path}`."

    out: list[str] = [f"### CPython `{rel_path}` matches for `{query}` (Found {len(matches)}):\n"]
    for idx in matches[:max_matches]:
        start = max(0, idx - 1)
        end = min(len(lines), idx + 2)
        out.append("```c")
        for line_num in range(start, end):
            marker = ">" if line_num == idx else " "
            out.append(f"{marker} {line_num + 1:5d} | {lines[line_num]}")
        out.append("```\n")

    return "\n".join(out)


@mcp.tool()
def list_cpython_aliases() -> str:
    """List all available canonical CPython runtime shortcuts and their file paths.

    Returns:
        Markdown table of available CPython source shortcuts.
    """
    lines: list[str] = [
        "### Canonical CPython Runtime Files\n",
        "| Alias | File Path |",
        "| :--- | :--- |",
    ]
    for alias, rel_path in sorted(CANONICAL_CPYTHON_FILES.items()):
        lines.append(f"| `{alias}` | `{rel_path}` |")
    return "\n".join(lines)


if __name__ == "__main__":
    mcp.run()
