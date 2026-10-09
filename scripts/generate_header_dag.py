#!/usr/bin/env python3
"""
Generate a Mermaid dependency graph from C header file #include directives.

Scans specified directories (defaults to src/ and include/) for header files (.h),
extracts internal header dependencies, detects any inclusion cycles, generates
a clean Mermaid flowchart diagram with clickable GitHub links, and optionally
writes it to an output file.
"""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(
    add_completion=False,
    help="Generate Mermaid graph of C header file dependencies.",
)


def discover_headers(search_dirs: list[Path]) -> dict[str, Path]:
    """Discover all .h files across search directories mapping header name to its Path."""
    headers: dict[str, Path] = {}
    for d in search_dirs:
        if not d.exists():
            continue
        for p in d.rglob("*.h"):
            headers[p.name] = p
    return headers


def parse_includes(header_path: Path, known_headers: set[str]) -> list[str]:
    """Parse #include "header.h" statements matching known internal project headers."""
    content = header_path.read_text(encoding="utf-8")
    includes: list[str] = []
    for match in re.finditer(r'#include\s+["<]([^">]+\.h)[">]', content):
        inc_target = Path(match.group(1)).name
        if (
            inc_target in known_headers
            and inc_target != header_path.name
            and inc_target not in includes
        ):
            includes.append(inc_target)
    return sorted(includes)


def detect_cycles(adj: dict[str, list[str]]) -> list[list[str]]:
    """Detect cycles in the dependency graph using depth-first search."""
    cycles: list[list[str]] = []
    visited: dict[str, int] = {}  # 0: unvisited, 1: visiting, 2: visited
    stack: list[str] = []

    def dfs(node: str) -> None:
        visited[node] = 1
        stack.append(node)
        for neighbor in adj.get(node, []):
            if visited.get(neighbor, 0) == 1:
                idx = stack.index(neighbor)
                cycles.append(stack[idx:] + [neighbor])
            elif visited.get(neighbor, 0) == 0:
                dfs(neighbor)
        stack.pop()
        visited[node] = 2

    for node in sorted(adj.keys()):
        if visited.get(node, 0) == 0:
            dfs(node)

    return cycles


def generate_mermaid_graph(
    headers_map: dict[str, Path],
    adj: dict[str, list[str]],
    root_dir: Path,
    with_links: bool = True,
) -> str:
    """Generate Mermaid flowchart representation of header dependencies with clickable links."""
    lines: list[str] = ["flowchart TD"]

    dir_groups: dict[str, list[str]] = defaultdict(list)
    for name, p in sorted(headers_map.items()):
        rel_dir = p.parent.relative_to(root_dir)
        dir_groups[str(rel_dir)].append(name)

    node_id_map: dict[str, str] = {}
    for dir_path, header_names in sorted(dir_groups.items()):
        safe_dir_id = re.sub(r"[^a-zA-Z0-9_]", "_", dir_path)
        lines.append(f'  subgraph sg_{safe_dir_id} ["{dir_path}/"]')
        for h_name in sorted(header_names):
            node_id = "h_" + re.sub(r"[^a-zA-Z0-9_]", "_", h_name)
            node_id_map[h_name] = node_id
            lines.append(f'    {node_id}["{h_name}"]')
        lines.append("  end")

    lines.append("")
    has_edges = False
    for src_header in sorted(adj.keys()):
        src_id = node_id_map.get(src_header, src_header)
        for dst_header in sorted(adj[src_header]):
            dst_id = node_id_map.get(dst_header, dst_header)
            lines.append(f"  {src_id} --> {dst_id}")
            has_edges = True

    if not has_edges:
        lines.append("  %% No internal dependencies found")

    if with_links:
        lines.append("")
        for h_name, p in sorted(headers_map.items()):
            node_id = node_id_map[h_name]
            rel_path = p.relative_to(root_dir).as_posix()
            lines.append(f'  click {node_id} "{rel_path}" "Jump to {h_name}"')

    return "\n".join(lines)


@app.command()
def main(
    dirs: Annotated[
        list[str] | None,
        typer.Argument(help="Directories to search for header files (default: src include)."),
    ] = None,
    output: Annotated[
        Path | None,
        typer.Option(
            "--output",
            "-o",
            help="Path to save the generated Mermaid graph file (e.g. docs/header_dag.mmd).",
        ),
    ] = None,
    with_links: Annotated[
        bool,
        typer.Option(
            "--with-links/--no-links",
            help="Include interactive clickable Markdown/GitHub links on nodes.",
        ),
    ] = True,
    check_dag: Annotated[
        bool,
        typer.Option(
            "--check-dag",
            help="Exit with non-zero status code if dependency cycles are detected.",
        ),
    ] = False,
) -> None:
    """Generate Mermaid graph of C header file dependencies."""
    search_dir_names = dirs if dirs else ["src", "include"]
    project_root = Path.cwd()
    search_dirs = [project_root / d for d in search_dir_names]

    headers_map = discover_headers(search_dirs)
    if not headers_map:
        typer.echo("No header files found in specified directories.", err=True)
        raise typer.Exit(code=1)

    known_headers = set(headers_map.keys())
    adj: dict[str, list[str]] = {}
    for name, path in headers_map.items():
        adj[name] = parse_includes(path, known_headers)

    cycles = detect_cycles(adj)
    if cycles:
        typer.echo("⚠️ Warning: Dependency cycle(s) detected in header files:", err=True)
        for c in cycles:
            typer.echo(f"  {' -> '.join(c)}", err=True)
        if check_dag:
            raise typer.Exit(code=1)

    mermaid_diagram = generate_mermaid_graph(headers_map, adj, project_root, with_links=with_links)

    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(mermaid_diagram + "\n", encoding="utf-8")
        typer.echo(f"✓ Saved Mermaid header DAG to {output}")
    else:
        typer.echo(mermaid_diagram)


if __name__ == "__main__":
    app()
