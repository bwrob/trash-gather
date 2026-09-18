#!/usr/bin/env python3
"""
Generate and synchronize the Mermaid DAG in roadmap/README.md from roadmap milestone writeups.
Applies Transitive Reduction to eliminate redundant cross-tier shortcut edges while
preserving all prerequisite definitions in the milestone markdown files.
"""

import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


def load_milestones(roadmap_dir: Path) -> dict[str, dict[str, Any]]:
    """Load all milestone metadata and prerequisites from markdown files."""
    milestones: dict[str, dict[str, Any]] = {}
    for fpath in sorted(roadmap_dir.glob("*_*.md")):
        content = fpath.read_text(encoding="utf-8")
        id_m = re.search(r"\*\*ID:\*\*\s*`([a-f0-9]{7})`", content)
        if not id_m:
            continue
        m_id = id_m.group(1)
        diff_m = re.search(r"\*\*Difficulty:\*\*\s*(\d+)", content)
        diff = int(diff_m.group(1)) if diff_m else 1
        status_m = re.search(r"\*\*Status:\*\*\s*(.+)", content)
        status = status_m.group(1).strip() if status_m else "Planned"
        slug_m = re.match(r"([a-f0-9]{7})_(.+)\.md", fpath.name)
        slug = slug_m.group(2) if slug_m else fpath.stem
        prereq_match = re.search(r"\*\*Prerequisites:\*\*\s*(.+)", content)
        prereq_ids: list[str] = []
        if prereq_match:
            prereqs = re.findall(r"\[([^\]]+)\]\(([a-f0-9]{7})_[^)]+\.md\)", prereq_match.group(1))
            prereq_ids = [p[1] for p in prereqs]

        milestones[m_id] = {
            "id": m_id,
            "slug": slug,
            "diff": diff,
            "status": status,
            "prereqs": prereq_ids,
            "file": fpath,
        }
    return milestones


def _build_reachability(adj: dict[str, set[str]], nodes: set[str]) -> dict[str, set[str]]:
    """Compute transitive reachability sets for each node in the DAG."""
    reach: dict[str, set[str]] = {}

    def get_reachable(u: str) -> set[str]:
        if u in reach:
            return reach[u]
        res: set[str] = set()
        for v in adj[u]:
            res.add(v)
            res.update(get_reachable(v))
        reach[u] = res
        return res

    for n in nodes:
        get_reachable(n)
    return reach


def _is_redundant_edge(
    u: str, v: str, adj: dict[str, set[str]], reach: dict[str, set[str]]
) -> bool:
    """Check if direct edge u -> v has an alternate path u -> w ~> v."""
    return any(w != v and v in reach[w] for w in adj[u])


def compute_transitive_reduction(
    milestones: dict[str, dict[str, Any]],
) -> list[tuple[str, str]]:
    """Compute the transitive reduction of the DAG: remove (u -> v) if path u ~> v exists."""
    adj: dict[str, set[str]] = defaultdict(set)
    for m_id, data in milestones.items():
        prereqs: list[str] = data["prereqs"]
        for p in prereqs:
            adj[p].add(m_id)

    all_nodes = set(milestones.keys())
    reach = _build_reachability(adj, all_nodes)

    reduced_edges: list[tuple[str, str]] = []
    for u in sorted(all_nodes):
        for v in sorted(adj[u]):
            if not _is_redundant_edge(u, v, adj, reach):
                reduced_edges.append((u, v))

    return reduced_edges


def update_roadmap_readme(roadmap_readme: Path, reduced_edges: list[tuple[str, str]]) -> None:
    """Update only the edges block in the Mermaid diagram inside roadmap/README.md."""
    content = roadmap_readme.read_text(encoding="utf-8")

    # Map milestone hash to Mermaid node identifier (e.g. "d9c6780" -> "m_d9c6780")
    node_names: dict[str, str] = {}
    for line in content.splitlines():
        m1 = re.search(r"(m_[a-f0-9]{7})\[", line)
        if m1:
            node_names[m1.group(1)[2:]] = m1.group(1)
            continue
        m2 = re.search(r"(\w+)\[\"([a-f0-9]{7}):", line)
        if m2:
            node_names[m2.group(2)] = m2.group(1)

    # Sort edges by Mermaid node names
    def edge_sort_key(edge: tuple[str, str]) -> tuple[str, str]:
        return (node_names.get(edge[0], edge[0]), node_names.get(edge[1], edge[1]))

    edge_lines: list[str] = []
    for u, v in sorted(reduced_edges, key=edge_sort_key):
        if u in node_names and v in node_names:
            edge_lines.append(f"  {node_names[u]} --> {node_names[v]}")

    formatted_edges = "\n".join(edge_lines)

    # Replace the edge section in the Mermaid block
    pattern = r"(\n  subgraph Tier5 .*?end\s*\n  end\s*\n\n)(.*?)(\n\s*Tier0 ~~~ Tier1)"
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print(
            "ERROR: Could not locate edge block in roadmap/README.md Mermaid diagram",
            file=sys.stderr,
        )
        sys.exit(1)

    new_content = content[: match.start(2)] + formatted_edges + content[match.end(2) :]
    roadmap_readme.write_text(new_content, encoding="utf-8")
    print(f"✓ Synchronized Mermaid diagram: {len(edge_lines)} transitively reduced edges.")


def main() -> None:
    roadmap_dir = Path("roadmap")
    roadmap_readme = roadmap_dir / "README.md"
    milestones = load_milestones(roadmap_dir)
    reduced_edges = compute_transitive_reduction(milestones)
    update_roadmap_readme(roadmap_readme, reduced_edges)


if __name__ == "__main__":
    main()
