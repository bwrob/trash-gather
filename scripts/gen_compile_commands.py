#!/usr/bin/env python3
"""
Generate compile_commands.json for clangd / language servers in trash-gather.
Enables full jump-to-definition, type hover, auto-completion, and inline diagnostics.
"""

import glob
import json
import os


def generate_compile_commands():
    workspace_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    cflags = (
        "gcc -Wall -Wextra -std=c99 -g -fsanitize=address,undefined "
        "-Iinclude -Isrc -Ivendor/munit -Ivendor/bootlib -include bootlib.h"
    )
    bench_flags = (
        "clang++ -O3 -std=c++17 -fsanitize=address,undefined "
        "-Iinclude -Isrc -Ivendor/bootlib -I/opt/homebrew/opt/google-benchmark/include"
    )

    entries = []

    # C source files in src, tests, vendor
    c_files = (
        glob.glob(os.path.join(workspace_root, "src", "*.c"))
        + glob.glob(os.path.join(workspace_root, "tests", "*.c"))
        + [
            os.path.join(workspace_root, "vendor", "bootlib", "bootlib.c"),
            os.path.join(workspace_root, "vendor", "munit", "munit.c"),
        ]
    )

    for src in sorted(c_files):
        rel_path = os.path.relpath(src, workspace_root)
        entries.append(
            {
                "directory": workspace_root,
                "command": f"{cflags} -c {rel_path}",
                "file": rel_path,
            }
        )

    # C++ benchmark files
    cpp_files = glob.glob(os.path.join(workspace_root, "bench", "*.cpp"))
    for bench in sorted(cpp_files):
        rel_path = os.path.relpath(bench, workspace_root)
        entries.append(
            {
                "directory": workspace_root,
                "command": f"{bench_flags} -c {rel_path}",
                "file": rel_path,
            }
        )

    output_path = os.path.join(workspace_root, "compile_commands.json")
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(entries, f, indent=2)

    print(
        f"Generated compile_commands.json ({len(entries)} translation units) for clangd / LSP."
    )


if __name__ == "__main__":
    generate_compile_commands()
