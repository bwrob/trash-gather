#!/usr/bin/env python3
"""
Synchronize the C header dependency Mermaid DAG in the root README.md.

Extracts the Mermaid diagram from a source file (e.g. docs/header_dag.mmd) or
generates it on-the-fly, and updates the delimited section in README.md:
<!-- HEADER_DAG_START -->
```mermaid
...
```
<!-- HEADER_DAG_END -->

Provides a --check flag for CI / pre-commit validation.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(
    add_completion=False,
    help="Synchronize the C header dependency Mermaid DAG in the root README.md.",
)

START_MARKER = "<!-- HEADER_DAG_START -->"
END_MARKER = "<!-- HEADER_DAG_END -->"


def format_dag_section(mermaid_content: str) -> str:
    """Format the markdown replacement block with HTML comment markers."""
    clean_mermaid = mermaid_content.strip()
    return f"{START_MARKER}\n```mermaid\n{clean_mermaid}\n```\n{END_MARKER}"


@app.command()
def main(
    readme: Annotated[
        Path,
        typer.Option(
            "--readme",
            "-r",
            help="Path to the target README file to update.",
        ),
    ] = Path("README.md"),
    dag_file: Annotated[
        Path,
        typer.Option(
            "--dag-file",
            "-d",
            help="Path to the saved Mermaid DAG file (e.g. docs/header_dag.mmd).",
        ),
    ] = Path("docs/header_dag.mmd"),
    check: Annotated[
        bool,
        typer.Option(
            "--check",
            "-c",
            help="Check if README is up-to-date without modifying it (exits 1 if out-of-sync).",
        ),
    ] = False,
) -> None:
    """Synchronize the header Mermaid DAG in README.md."""
    if not readme.exists():
        typer.echo(f"ERROR: README file '{readme}' does not exist.", err=True)
        raise typer.Exit(code=1)

    if not dag_file.exists():
        typer.echo(f"ERROR: Header DAG file '{dag_file}' does not exist.", err=True)
        raise typer.Exit(code=1)

    dag_content = dag_file.read_text(encoding="utf-8")
    readme_content = readme.read_text(encoding="utf-8")

    expected_block = format_dag_section(dag_content)
    pattern = re.compile(
        rf"{re.escape(START_MARKER)}.*?{re.escape(END_MARKER)}",
        re.DOTALL,
    )

    if not pattern.search(readme_content):
        typer.echo(
            f"ERROR: Delimiters '{START_MARKER}' and '{END_MARKER}' not found in {readme}.",
            err=True,
        )
        raise typer.Exit(code=1)

    current_match = pattern.search(readme_content)
    assert current_match is not None  # Guaranteed by check above

    if current_match.group(0) == expected_block:
        if check:
            typer.echo(f"✓ {readme} header DAG is up-to-date.")
        return

    if check:
        typer.echo(
            f"❌ {readme} header DAG is out-of-date. Run 'just update-header-dag' to sync.",
            err=True,
        )
        raise typer.Exit(code=1)

    new_readme_content = pattern.sub(expected_block, readme_content)
    readme.write_text(new_readme_content, encoding="utf-8")
    typer.echo(f"✓ Updated header DAG in {readme}.")


if __name__ == "__main__":
    app()
