#!/usr/bin/env python3
"""
Header Self-Containment Linter.

Compiles each C header file in isolation with -fsyntax-only and -Werror to ensure
that headers include all required type definitions (<stddef.h>, <stdint.h>, etc.)
and do not rely on implicit inclusion order from translation units.
"""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from typing import Annotated

import typer
from project_config import get_c_standard

app = typer.Typer(
    add_completion=False,
    help="Verify that each C header is self-contained and compiles in isolation.",
)


def _get_src_inc_flags(repo_root: Path) -> list[str]:
    """Return -I flags for include/, src/, and all nested subdirectories."""
    flags: list[str] = []
    include_dir = repo_root / "include"
    if include_dir.exists():
        flags.append(f"-I{include_dir}")

    src_dir = repo_root / "src"
    if src_dir.exists():
        for root, _, _ in os.walk(src_dir):
            flags.append(f"-I{root}")
    return sorted(flags)


def check_header(header_path: Path, inc_flags: list[str], c_std: str) -> bool:
    """Compile a single header with -fsyntax-only wrapped as an include in a C TU."""
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        f"-std={c_std}",
        "-fsyntax-only",
        *inc_flags,
        "-x",
        "c",
        "-",
    ]

    include_stmt = f'#include "{header_path.as_posix()}"\n'
    result = subprocess.run(cmd, input=include_stmt, capture_output=True, text=True)
    if result.returncode != 0:
        typer.echo(f"❌ {header_path} is not self-contained:", err=True)
        if result.stderr:
            typer.echo(result.stderr.strip(), err=True)
        return False

    return True


@app.command()
def main(
    dirs: Annotated[
        list[str] | None,
        typer.Argument(help="Directories to search for header files (default: src include)."),
    ] = None,
) -> None:
    """Check that all C header files compile cleanly in total isolation."""
    search_dir_names = dirs if dirs else ["src", "include"]
    repo_root = Path.cwd()
    search_dirs = [repo_root / d for d in search_dir_names]

    headers: list[Path] = []
    for d in search_dirs:
        if d.exists():
            headers.extend(d.rglob("*.h"))

    if not headers:
        typer.echo("No header files found in specified directories.")
        return

    inc_flags = _get_src_inc_flags(repo_root)
    c_std = get_c_standard()

    failures = 0
    for h in sorted(headers):
        rel_h = h.relative_to(repo_root)
        if not check_header(h, inc_flags, c_std):
            failures += 1
        else:
            typer.echo(f"  ✓ {rel_h} is self-contained")

    if failures > 0:
        typer.echo(
            f"\n❌ Header self-containment check failed ({failures} file(s) broken).", err=True
        )
        raise typer.Exit(code=1)

    typer.echo(f"\n✅ All {len(headers)} headers are cleanly self-contained!")


if __name__ == "__main__":
    app()
