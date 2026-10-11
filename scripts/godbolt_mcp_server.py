#!/usr/bin/env python3
"""Model Context Protocol (MCP) Server for Compiler Explorer (Godbolt).

Exposes online C compiler assembly generation and diagnostic inspection tools
over stdio to explore machine instruction output, loop vectorization, and register usage.
"""

from __future__ import annotations

import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

_scripts_dir = str(Path(__file__).resolve().parent)
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)

from godbolt import format_compilation_result, query_godbolt_compilation  # noqa: E402

mcp = FastMCP("godbolt-compiler-explorer")


@mcp.tool()
def inspect_c_assembly(
    code: str,
    compiler: str = "cclang1810",
    options: str = "-O3 -std=c17",
) -> str:
    """Compile C code on Compiler Explorer (Godbolt) and return generated assembly and diagnostics.

    Args:
        code: The complete C function or snippet to compile.
        compiler: Target compiler ID ('cclang1810' for Clang 18.1, 'cg141' for GCC 14.1,
            'aarch64-gcc' for ARM64).
        options: Compiler flags (defaults to '-O3 -std=c17').

    Returns:
        Formatted assembly output with compiler diagnostics and warnings.
    """
    res = query_godbolt_compilation(code, compiler, options)
    return format_compilation_result(res, compiler, options)


if __name__ == "__main__":
    mcp.run()
