#!/usr/bin/env python3
"""Model Context Protocol (MCP) Server for Struct Memory Layout Inspection.

Exposes Clang AST record layout inspection tools over stdio to verify byte offsets,
struct padding holes, and alignment boundaries on physical host silicon.
"""

from __future__ import annotations

import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

_scripts_dir = str(Path(__file__).resolve().parent)
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)

from struct_layout import WORKSPACE_ROOT, dump_record_layouts  # noqa: E402

mcp = FastMCP("struct-layout-inspector")


@mcp.tool()
def inspect_struct_layout(
    struct_name: str,
    file: str = "src/vm_objects/object.c",
) -> str:
    """Inspect the exact byte offsets, field sizes, and padding holes of a C struct.

    Args:
        struct_name: The identifier of the struct or union
            (e.g. 'struct Object', 'list_t', 'tuple_t').
        file: Relative path to the C file (defaults to 'src/vm_objects/object.c').

    Returns:
        Exact AST record layout showing byte offsets, padding, sizeof, and alignment.
    """
    source_path = WORKSPACE_ROOT / file
    if not source_path.is_file():
        return f"Error: Source file '{file}' does not exist in workspace."

    records = dump_record_layouts(source_path)
    matched_key: str | None = None
    for k in records:
        if struct_name.lower() in k.lower():
            matched_key = k
            break

    if not matched_key:
        available = ", ".join(records.keys())
        return f"Struct '{struct_name}' not found in {file}. Available records: {available}"

    layout = records[matched_key]
    return f"### AST Memory Layout for `{matched_key}` in `{file}`\n\n```text\n{layout}\n```"


@mcp.tool()
def list_struct_records(file: str = "src/vm_objects/object.c") -> str:
    """List all structs and record definitions in a C source file with sizeof and alignment.

    Args:
        file: Relative path to the C file (defaults to 'src/vm_objects/object.c').

    Returns:
        Markdown table of all records with their byte size and alignment.
    """
    source_path = WORKSPACE_ROOT / file
    if not source_path.is_file():
        return f"Error: Source file '{file}' does not exist."

    records = dump_record_layouts(source_path)
    if not records:
        return f"No records found in {file}."

    lines: list[str] = [
        f"### Structs and Records in `{file}`\n",
        "| Record Name | Size / Alignment |",
        "| :--- | :--- |",
    ]
    for name, content in records.items():
        size_lines = [ln.strip() for ln in content.splitlines() if "sizeof=" in ln]
        meta = size_lines[0] if size_lines else "N/A"
        lines.append(f"| `{name}` | `{meta}` |")

    return "\n".join(lines)


if __name__ == "__main__":
    mcp.run()
