#!/usr/bin/env python3
"""Struct Memory Layout & Alignment Inspector.

Uses Clang's AST layout engine (-Xclang -fdump-record-layouts) to extract
exact byte offsets, field sizes, alignment holes, and cache-line footprints
for structs and unions in the C runtime.
"""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(
    add_completion=False,
    help="Inspect exact byte-level memory layouts of C structs and unions.",
)

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent


def get_include_flags() -> list[str]:
    """Return clang include flags for the workspace."""
    inc_dirs = ["include", "src", "src/vm", "src/vm_objects", "vendor/munit", "vendor/bootlib"]
    flags: list[str] = []
    for d in inc_dirs:
        p = WORKSPACE_ROOT / d
        if p.is_dir():
            flags.extend(["-I", str(p)])
    return flags


def dump_record_layouts(source_file: Path) -> dict[str, str]:
    """Invoke clang and extract all AST record layout blocks."""
    if not source_file.is_file():
        return {}

    cmd: list[str] = [
        "clang",
        "-std=c17",
        *get_include_flags(),
        "-include",
        "bootlib.h",
        "-Xclang",
        "-fdump-record-layouts",
        "-fsyntax-only",
        str(source_file),
    ]

    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    output = proc.stdout + proc.stderr
    blocks = output.split("*** Dumping AST Record Layout")

    records: dict[str, str] = {}
    for block in blocks:
        stripped = block.strip()
        lines = stripped.splitlines()
        if not lines:
            continue
        first_line = lines[0].strip()
        name = first_line.split("|")[-1].strip() if "|" in first_line else first_line
        if name:
            records[name] = stripped

    return records


@app.command()
def inspect(
    struct_name: Annotated[
        str,
        typer.Argument(help="Name of struct or union (e.g. 'struct Object', 'list_t')"),
    ],
    file: Annotated[
        str,
        typer.Option("--file", "-f", help="C source file to analyze"),
    ] = "src/vm_objects/object.c",
) -> None:
    """Inspect the memory layout, byte offsets, and padding of a struct."""
    source_path = WORKSPACE_ROOT / file
    records = dump_record_layouts(source_path)

    matched_key: str | None = None
    for k in records:
        if struct_name.lower() in k.lower():
            matched_key = k
            break

    if not matched_key:
        typer.secho(
            f"Struct or union '{struct_name}' not found in {file}.",
            fg=typer.colors.RED,
            err=True,
        )
        typer.echo(f"Available records: {', '.join(records.keys())}")
        raise typer.Exit(code=1)

    typer.secho(f"\nExact AST Memory Layout for: {matched_key}\n", fg=typer.colors.CYAN, bold=True)
    typer.echo(records[matched_key])
    typer.echo()


@app.command()
def list_records(
    file: Annotated[
        str,
        typer.Option("--file", "-f", help="C source file to analyze"),
    ] = "src/vm_objects/object.c",
) -> None:
    """List all structs and unions defined in the specified file."""
    source_path = WORKSPACE_ROOT / file
    records = dump_record_layouts(source_path)

    if not records:
        typer.secho(f"No records found in {file}.", fg=typer.colors.YELLOW)
        return

    typer.secho(f"Discovered {len(records)} records in {file}:\n", fg=typer.colors.CYAN)
    for name, content in records.items():
        size_line = [ln for ln in content.splitlines() if "sizeof=" in ln]
        meta = size_line[0].strip() if size_line else ""
        typer.echo(f"  • {name:<30} {meta}")


if __name__ == "__main__":
    app()
