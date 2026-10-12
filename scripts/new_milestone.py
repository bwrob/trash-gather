#!/usr/bin/env python3
"""
Milestone Scaffolding Utility.

Automates the creation of new roadmap milestones by:
1. Validating the milestone slug.
2. Computing the deterministic 7-character sha256 hash ID.
3. Instantiating the 5-section template from resources/template.md.
4. Outputting clear instructions for integrating into the Mermaid DAG.
"""

from __future__ import annotations

import hashlib
import re
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(
    add_completion=False,
    help="Scaffold a new roadmap milestone with deterministic hash ID and template.",
)


def slugify(text: str) -> str:
    """Normalize text into a clean snake_case slug."""
    text = text.lower().strip()
    text = re.sub(r"[^\w\s-]", "", text)
    text = re.sub(r"[-\s]+", "_", text)
    return text.strip("_")


def title_case(slug: str) -> str:
    """Convert snake_case slug into a human-readable title."""
    return " ".join(word.capitalize() for word in slug.split("_"))


@app.command()
def main(
    slug: Annotated[
        str,
        typer.Argument(help="Milestone slug in snake_case (e.g., 'string_interning')."),
    ],
    title: Annotated[
        str | None,
        typer.Argument(help="Human-readable milestone title (defaults to Title Cased Slug)."),
    ] = None,
    difficulty: Annotated[
        int,
        typer.Option(
            "--difficulty",
            "-d",
            min=1,
            max=5,
            help="Difficulty level from 1 (entry) to 5 (expert), default: 3.",
        ),
    ] = 3,
) -> None:
    """Scaffold a new roadmap milestone file."""
    clean_slug = slugify(slug)
    if not clean_slug:
        typer.echo("ERROR: Slug cannot be empty.", err=True)
        raise typer.Exit(code=1)

    clean_title = title.strip() if title else title_case(clean_slug)
    hash_id = hashlib.sha256(clean_slug.encode("utf-8")).hexdigest()[:7]

    repo_root = Path(__file__).resolve().parent.parent
    roadmap_dir = repo_root / "roadmap"
    template_path = (
        repo_root / ".agents" / "skills" / "roadmap-milestone" / "resources" / "template.md"
    )

    if not template_path.exists():
        typer.echo(f"ERROR: Template file not found at '{template_path}'", err=True)
        raise typer.Exit(code=1)

    target_file = roadmap_dir / f"{hash_id}_{clean_slug}.md"
    if target_file.exists():
        typer.echo(
            f"ERROR: Milestone file '{target_file}' already exists!",
            err=True,
        )
        raise typer.Exit(code=1)

    template_content = template_path.read_text(encoding="utf-8")
    content = template_content.replace("<hash_id>", hash_id)
    content = content.replace("<Milestone Title>", clean_title)
    content = content.replace("<1-5>", str(difficulty))

    target_file.write_text(content, encoding="utf-8")
    typer.echo(f"✨ Created milestone writeup: {target_file}")
    typer.echo(f"   ID:         {hash_id}")
    typer.echo(f"   Slug:       {clean_slug}")
    typer.echo(f"   Title:      {clean_title}")
    typer.echo(f"   Difficulty: {difficulty} / 5\n")
    typer.echo("Next Steps:")
    typer.echo(f"1. Open '{target_file}' and fill out Sections 1 through 6.")
    typer.echo("2. Open 'roadmap/README.md':")
    node_str = f'm_{clean_slug}["{hash_id}: {clean_title} (Diff: {difficulty})"]:::planned'
    typer.echo(f"   - Add node: {node_str} to the matching difficulty tier.")
    typer.echo(f"   - Connect prerequisite edges (e.g., m_parent --> m_{clean_slug}).")
    typer.echo("   - Add milestone entry to the Tier progression index.")
    typer.echo("3. Run 'just lint-roadmap' to verify DAG integrity and link consistency.")


if __name__ == "__main__":
    app()
