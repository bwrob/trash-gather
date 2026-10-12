#!/usr/bin/env python3
"""Compiler Explorer (Godbolt) Assembly Inspector.

Compiles short ISO C17 snippets against various compilers (Clang, GCC, ARM64)
using the Godbolt API to inspect generated assembly, branch elimination,
and register allocation.
"""

from __future__ import annotations

from typing import Annotated, Any

import requests
import typer

app = typer.Typer(
    add_completion=False,
    help="Compile C snippets on Compiler Explorer (Godbolt) and inspect machine assembly.",
)

GODBOLT_API_URL: str = "https://godbolt.org/api"
DEFAULT_COMPILER: str = "cclang1810"  # Clang 18.1.0


def query_godbolt_compilation(
    code: str,
    compiler_id: str = DEFAULT_COMPILER,
    options: str = "-O3 -std=c17",
) -> dict[str, Any]:
    """Submit code to Godbolt API and return parsed assembly and diagnostics."""
    endpoint = f"{GODBOLT_API_URL}/compiler/{compiler_id}/compile"
    headers = {"Accept": "application/json"}
    payload = {
        "source": code,
        "options": {
            "userArguments": options,
            "compilerOptions": {"skipAsm": False, "executorRequest": False},
            "filters": {
                "intel": True,
                "demangle": True,
                "directives": True,
                "comments": True,
                "labels": True,
            },
        },
    }

    try:
        response = requests.post(endpoint, json=payload, headers=headers, timeout=15)
        response.raise_for_status()
        res_data: dict[str, Any] = response.json()
        return res_data
    except Exception as exc:  # noqa: BLE001
        return {"error": str(exc)}


def format_compilation_result(result: dict[str, Any], compiler_id: str, options: str) -> str:
    """Format Godbolt compilation JSON into human-readable assembly output."""
    if "error" in result:
        return f"Error connecting to Compiler Explorer: {result['error']}"

    lines: list[str] = [f"### Godbolt Output ({compiler_id} with `{options}`)\n"]

    # Compiler stderr / diagnostics
    stderr_lines = [item.get("text", "") for item in result.get("stderr", []) if item.get("text")]
    if stderr_lines:
        lines.append("**Compiler Diagnostics:**\n```text")
        lines.extend(stderr_lines)
        lines.append("```\n")

    # Assembly output
    asm_lines = [item.get("text", "") for item in result.get("asm", [])]
    if asm_lines:
        lines.append("**Generated Assembly:**\n```nasm")
        lines.extend(asm_lines)
        lines.append("```")
    else:
        lines.append("No assembly generated (compilation may have failed).")

    return "\n".join(lines)


@app.command()
def compile_code(
    code: Annotated[str, typer.Argument(help="C source code to compile")],
    compiler: Annotated[
        str,
        typer.Option("--compiler", "-c", help="Compiler ID (e.g. 'cclang1810', 'cg141')"),
    ] = DEFAULT_COMPILER,
    options: Annotated[
        str,
        typer.Option("--options", "-o", help="Compiler flags (e.g. '-O3 -std=c17')"),
    ] = "-O3 -std=c17",
) -> None:
    """Compile C snippet via Godbolt and display assembly."""
    result = query_godbolt_compilation(code, compiler, options)
    formatted = format_compilation_result(result, compiler, options)
    typer.echo(formatted)


if __name__ == "__main__":
    app()
