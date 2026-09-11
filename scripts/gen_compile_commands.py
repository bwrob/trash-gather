#!/usr/bin/env python3
"""
Generate compile_commands.json for clangd / language servers in trash-gather.
Enables full jump-to-definition, type hover, auto-completion, and inline diagnostics.
"""

import glob
import json
import os

from project_config import get_c_standard, get_cpp_standard


def generate_compile_commands() -> None:
    workspace_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    c_std = get_c_standard()
    cpp_std = get_cpp_standard()

    cflags = (
        f"gcc -Wall -Wextra -std={c_std} -g -fsanitize=address,undefined "
        "-Iinclude -Isrc -Ivendor/munit -Ivendor/bootlib -include bootlib.h"
    )
    bench_paths = [
        "/opt/homebrew/opt/google-benchmark/include",
        "/usr/local/opt/google-benchmark/include",
        "/usr/include",
        "/usr/local/include",
    ]
    extra_inc = " ".join(["-I" + p for p in bench_paths if os.path.isdir(p)])

    bench_flags = (
        f"clang++ -O3 -std={cpp_std} -fsanitize=address,undefined "
        f"-Iinclude -Isrc -Ivendor/bootlib {extra_inc}"
    )

    entries: list[dict[str, str]] = []

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
        cmd = cflags if "munit.c" not in src else cflags.replace(" -include bootlib.h", "")
        entries.append(
            {
                "directory": workspace_root,
                "command": f"{cmd} -c {rel_path}",
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

    print(f"Generated compile_commands.json ({len(entries)} translation units) for clangd / LSP.")


if __name__ == "__main__":
    generate_compile_commands()
