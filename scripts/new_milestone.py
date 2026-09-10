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

import argparse
import hashlib
import re
import sys
from pathlib import Path


def slugify(text: str) -> str:
    """Normalize text into a clean snake_case slug."""
    text = text.lower().strip()
    text = re.sub(r"[^\w\s-]", "", text)
    text = re.sub(r"[-\s]+", "_", text)
    return text.strip("_")


def title_case(slug: str) -> str:
    """Convert snake_case slug into a human-readable title."""
    return " ".join(word.capitalize() for word in slug.split("_"))


def main() -> int:
    """Scaffold a new roadmap milestone file."""
    parser = argparse.ArgumentParser(
        description="Scaffold a new roadmap milestone with deterministic hash ID and template."
    )
    parser.add_argument(
        "slug",
        help="Milestone slug in snake_case (e.g., 'string_interning')",
    )
    parser.add_argument(
        "title",
        nargs="?",
        default=None,
        help="Human-readable milestone title (defaults to Title Cased Slug)",
    )

    args = parser.parse_args()
    slug = slugify(args.slug)
    if not slug:
        print("ERROR: Slug cannot be empty.", file=sys.stderr)
        return 1

    title = args.title.strip() if args.title else title_case(slug)
    hash_id = hashlib.sha256(slug.encode("utf-8")).hexdigest()[:7]

    repo_root = Path(__file__).resolve().parent.parent
    roadmap_dir = repo_root / "roadmap"
    template_path = (
        repo_root / ".agents" / "skills" / "roadmap-milestone" / "resources" / "template.md"
    )

    if not template_path.exists():
        print(f"ERROR: Template file not found at '{template_path}'", file=sys.stderr)
        return 1

    target_file = roadmap_dir / f"{hash_id}_{slug}.md"
    if target_file.exists():
        print(
            f"ERROR: Milestone file '{target_file}' already exists!",
            file=sys.stderr,
        )
        return 1

    template_content = template_path.read_text(encoding="utf-8")
    content = template_content.replace("<hash_id>", hash_id)
    content = content.replace("<Milestone Title>", title)

    target_file.write_text(content, encoding="utf-8")
    print(f"✨ Created milestone writeup: {target_file}")
    print(f"   ID:    {hash_id}")
    print(f"   Slug:  {slug}")
    print(f"   Title: {title}\n")
    print("Next Steps:")
    print(f"1. Open '{target_file}' and fill out Sections 1 through 5.")
    print("2. Open 'roadmap/README.md':")
    print(f'   - Add node: m_{slug}["{hash_id}: {title}"]:::planned to the appropriate track.')
    print(f"   - Connect prerequisite edges (e.g., m_parent --> m_{slug}).")
    print("   - Add milestone entry to the Track index.")
    print("3. Run 'just lint-roadmap' to verify DAG integrity and link consistency.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
