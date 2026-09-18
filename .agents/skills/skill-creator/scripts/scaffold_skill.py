#!/usr/bin/env python3
"""
Skill Scaffolding Utility.

Automates the creation of new workspace skills by:
1. Validating the skill name (lowercase, hyphenated).
2. Generating the standardized directory layout (.agents/skills/<name>/).
3. Rendering the starter SKILL.md from the skill template.
4. Setting up resources/, references/, and scripts/ subdirectories.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def validate_skill_name(name: str) -> str:
    """Validate and normalize a skill name to kebab-case."""
    clean = name.strip().lower()
    if not re.match(r"^[a-z0-9]+(-[a-z0-9]+)*$", clean):
        raise ValueError(
            f"Invalid skill name '{name}'. "
            "Must be lowercase kebab-case (e.g. 'c-expert', 'skill-creator')."
        )
    return clean


def to_title(name: str) -> str:
    """Convert kebab-case name into a human-readable title."""
    return " ".join(word.capitalize() for word in name.split("-"))


def scaffold_skill(
    skill_name: str,
    description: str | None = None,
    target_dir: Path | None = None,
) -> Path:
    """Scaffold a new skill directory with templates and subdirectories."""
    validated_name = validate_skill_name(skill_name)
    skill_title = to_title(validated_name)

    if target_dir is None:
        target_dir = Path(".agents/skills") / validated_name
    else:
        target_dir = target_dir / validated_name

    if target_dir.exists():
        raise FileExistsError(f"Skill directory already exists: {target_dir}")

    # Create directory tree
    resources_dir = target_dir / "resources"
    references_dir = target_dir / "references"
    scripts_dir = target_dir / "scripts"

    resources_dir.mkdir(parents=True, exist_ok=True)
    references_dir.mkdir(parents=True, exist_ok=True)
    scripts_dir.mkdir(parents=True, exist_ok=True)

    desc = (
        description
        if description
        else f"Provides workflows, patterns, and procedures for {skill_title.lower()}."
    )
    summary = (
        "This skill defines the methodology, procedures, and quality standards for "
        f"{skill_title.lower()}."
    )

    # Load template
    template_path = Path(__file__).resolve().parent.parent / "resources" / "skill_template.md"
    if template_path.exists():
        content = template_path.read_text(encoding="utf-8")
    else:
        content = (
            "---\n"
            "name: ${SKILL_NAME}\n"
            "description: >-\n"
            "  ${SKILL_DESCRIPTION}\n"
            "---\n\n"
            "# ${SKILL_TITLE} Skill (`${SKILL_NAME}`)\n\n"
            "${SKILL_SUMMARY}\n"
        )

    content = content.replace("${SKILL_NAME}", validated_name)
    content = content.replace("${SKILL_TITLE}", skill_title)
    content = content.replace("${SKILL_DESCRIPTION}", desc)
    content = content.replace("${SKILL_SUMMARY}", summary)

    skill_file = target_dir / "SKILL.md"
    skill_file.write_text(content, encoding="utf-8")

    return target_dir


def main() -> int:
    """CLI entrypoint for scaffolding a new skill."""
    parser = argparse.ArgumentParser(
        description="Scaffold a new Antigravity skill with standardized directory layout."
    )
    parser.add_argument(
        "name",
        help="Skill name in kebab-case (e.g. 'fuzz-testing', 'cache-analysis')",
    )
    parser.add_argument(
        "--description",
        "-d",
        default=None,
        help="Description for the skill frontmatter",
    )
    parser.add_argument(
        "--path",
        "-p",
        type=Path,
        default=None,
        help="Base directory to place the skill (default: .agents/skills/)",
    )

    args = parser.parse_args()
    try:
        created_path = scaffold_skill(args.name, args.description, args.path)
        print(f"✅ Successfully scaffolded new skill at: {created_path}")
        print(f"   - Main instructions: {created_path}/SKILL.md")
        print(f"   - Reusable templates: {created_path}/resources/")
        print(f"   - Reference guides:   {created_path}/references/")
        print(f"   - Automation scripts: {created_path}/scripts/")
        return 0
    except Exception as exc:
        print(f"❌ Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
