#!/usr/bin/env python3
"""
Roadmap Linter & DAG Consistency Validator.

Validates:
1. Hash integrity: Filename prefixes match sha256(slug)[:7].
2. Schema compliance: Required metadata fields and 5 numbered sections.
3. Link integrity: All markdown links resolve to existing files.
4. Mermaid graph consistency: All milestones are represented in the DAG.
5. Single source of truth: Root README links to roadmap/README.md.
"""

from __future__ import annotations

import glob
import hashlib
import os
import re
import sys
from pathlib import Path

REQUIRED_SECTIONS = [
    "## 1. Objective & Technical Scope",
    "## 2. Architectural Design & Invariants",
    "## 3. Systems Concepts & Guiding Questions",
    "## 4. Implementation Steps & Touchpoints",
    "## 5. Verification & Acceptance Criteria",
]

REQUIRED_METADATA = [
    "**ID:**",
    "**Status:**",
    "**Focus:**",
    "**Prerequisites:**",
]


def validate_links(file_path: Path, content: str) -> list[str]:
    """Validate that all local markdown links in content point to existing files."""
    errors: list[str] = []
    dir_path = file_path.parent

    # Find [text](target.md) or [text](target.md#anchor)
    links = re.findall(r"\[([^\]]+)\]\(([^)]+)\)", content)
    for text, target in links:
        if target.startswith("http://") or target.startswith("https://") or target.startswith("#"):
            continue
        # Strip anchor if present
        clean_target = target.split("#")[0]
        if not clean_target:
            continue
        resolved = (dir_path / clean_target).resolve()
        if not resolved.exists():
            errors.append(
                f"{file_path}: Broken link [{text}]({target}) -> {resolved} does not exist"
            )
    return errors


def validate_milestone_file(path: Path) -> tuple[str, list[str]]:
    """Validate an individual milestone writeup against schema and hash rules."""
    errors: list[str] = []
    filename = path.name

    match = re.match(r"^([a-f0-9]{7})_(.+)\.md$", filename)
    if not match:
        errors.append(
            f"{path}: Filename does not match pattern '<hash>_<slug>.md' (expected 7-hex hash)"
        )
        return "", errors

    hash_id, slug = match.groups()
    expected_hash = hashlib.sha256(slug.encode("utf-8")).hexdigest()[:7]
    if hash_id != expected_hash:
        errors.append(
            f"{path}: Hash mismatch for '{slug}' (got '{hash_id}', expected '{expected_hash}')"
        )

    content = path.read_text(encoding="utf-8")

    # Check title
    if not re.search(r"^# Milestone:\s*.+$", content, re.M):
        errors.append(f"{path}: Missing or malformed title line (expected '# Milestone: <Title>')")

    # Check metadata fields
    for meta in REQUIRED_METADATA:
        if meta not in content:
            errors.append(f"{path}: Missing metadata field '{meta}'")

    # Check ID match inside metadata
    id_match = re.search(r"^\*\*ID:\*\*\s*`?([a-f0-9]+)`?", content, re.M)
    if id_match:
        if id_match.group(1) != hash_id:
            errors.append(
                f"{path}: Metadata ID `{id_match.group(1)}` does not match hash `{hash_id}`"
            )
    else:
        errors.append(f"{path}: Could not parse **ID:** metadata line")

    # Check all 5 required sections
    for sec in REQUIRED_SECTIONS:
        if sec not in content:
            errors.append(f"{path}: Missing required section '{sec}'")

    # Check link validity
    link_errors = validate_links(path, content)
    errors.extend(link_errors)

    return hash_id, errors


def validate_roadmap_index(roadmap_readme: Path, milestone_hashes: dict[str, Path]) -> list[str]:
    """Validate roadmap/README.md links and Mermaid DAG completeness."""
    errors: list[str] = []
    if not roadmap_readme.exists():
        return [f"{roadmap_readme}: File does not exist"]

    content = roadmap_readme.read_text(encoding="utf-8")

    # Validate links in roadmap/README.md
    errors.extend(validate_links(roadmap_readme, content))

    # Parse Mermaid diagram
    mermaid_match = re.search(r"```mermaid\s*\n(.*?)\n```", content, re.DOTALL)
    if not mermaid_match:
        errors.append(f"{roadmap_readme}: Missing Mermaid diagram block")
        return errors

    mermaid_code = mermaid_match.group(1)
    # Find all node definitions containing hash IDs (e.g., m_...["d9c6780: ..."])
    nodes_in_mermaid = set(re.findall(r'\["([a-f0-9]{7}):\s*[^"]+"\]', mermaid_code))

    # Check that all milestone files are represented in the Mermaid DAG
    for hash_id, path in milestone_hashes.items():
        if hash_id not in nodes_in_mermaid:
            errors.append(
                f"{roadmap_readme}: Milestone '{hash_id}' ({path.name}) is missing from Mermaid DAG"
            )

    # Check that all Mermaid nodes correspond to real milestone files
    for node_hash in nodes_in_mermaid:
        if node_hash not in milestone_hashes:
            errors.append(
                f"{roadmap_readme}: Mermaid DAG contains unknown milestone hash '{node_hash}'"
            )

    return errors


def validate_root_readme(root_readme: Path, roadmap_readme: Path) -> list[str]:
    """Validate that root README links to roadmap/README.md as the single source of truth."""
    errors: list[str] = []
    if not root_readme.exists():
        return [f"{root_readme}: Root README.md does not exist"]

    content = root_readme.read_text(encoding="utf-8")
    errors.extend(validate_links(root_readme, content))

    rel_path = os.path.relpath(roadmap_readme, root_readme.parent)
    if rel_path not in content:
        errors.append(
            f"{root_readme}: Does not link to the roadmap single source of truth ('{rel_path}')"
        )

    return errors


def main() -> int:
    """Run roadmap validation across milestone files and indexes."""
    repo_root = Path(__file__).resolve().parent.parent
    roadmap_dir = repo_root / "roadmap"
    roadmap_readme = roadmap_dir / "README.md"
    root_readme = repo_root / "README.md"

    milestone_files = sorted(
        [Path(p) for p in glob.glob(str(roadmap_dir / "*_*.md")) if Path(p).name != "README.md"]
    )

    if not milestone_files:
        print("ERROR: No milestone files found in roadmap/", file=sys.stderr)
        return 1

    print(f"=== Validating {len(milestone_files)} Roadmap Milestones & DAG ===")
    all_errors: list[str] = []
    milestone_hashes: dict[str, Path] = {}

    for mf in milestone_files:
        hash_id, errs = validate_milestone_file(mf)
        if hash_id:
            milestone_hashes[hash_id] = mf
        all_errors.extend(errs)

    index_errors = validate_roadmap_index(roadmap_readme, milestone_hashes)
    all_errors.extend(index_errors)

    root_errors = validate_root_readme(root_readme, roadmap_readme)
    all_errors.extend(root_errors)

    if all_errors:
        print(f"FAILED: Found {len(all_errors)} roadmap validation errors:\n", file=sys.stderr)
        for err in all_errors:
            print(f"  ❌ {err}", file=sys.stderr)
        return 1

    print(f"  ✓ All {len(milestone_files)} milestone files conform to schema and hash ID rules.")
    print("  ✓ All markdown links resolve successfully.")
    print("  ✓ Mermaid DAG in roadmap/README.md is 100% synchronized with milestone files.")
    print("  ✓ Root README.md correctly references the single source of truth.")
    print("✅ Roadmap validation passed cleanly!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
