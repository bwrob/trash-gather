#!/usr/bin/env python3
"""
Docstring Linter for Bootlib C code.
Validates presence, completeness, and Doxygen formatting of docstrings in vendor/bootlib/.
"""

import os
import re
import sys
import subprocess

def run_clang_documentation_check(target_files):
    """Run clang with -Wdocumentation flags to catch Doxygen syntax/command errors."""
    print("=== Running Clang -Wdocumentation Linting ===")
    errors = 0
    cmd = [
        "clang",
        "-fsyntax-only",
        "-Wdocumentation",
        "-Wdocumentation-unknown-command",
        "-Wdocumentation-pedantic",
        "-std=c99",
        "-Iinclude",
        "-Isrc",
        "-Ivendor/munit",
        "-Ivendor/bootlib",
    ] + target_files

    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0 or res.stderr:
        doc_warnings = [line for line in res.stderr.splitlines() if "warning:" in line or "error:" in line]
        if doc_warnings:
            print("\n".join(doc_warnings))
            errors += len(doc_warnings)

    if errors == 0:
        print("  ✓ Clang -Wdocumentation syntax checks passed cleanly.")
    return errors


def lint_header_docstrings(header_path):
    """Verify presence and tag completeness (@brief, @param, @return) for header prototypes."""
    print(f"\n=== Linting Doxygen Docstrings in {os.path.basename(header_path)} ===")
    errors = 0

    with open(header_path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Check for @file header docblock
    if not re.search(r"/\*\*.*?@file\b.*?\*/", content, re.DOTALL):
        print(f"ERROR: {header_path}: Missing top-level @file docstring block.")
        errors += 1

    # 2. Extract function prototypes and preceding comments
    lines = content.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        proto_match = re.match(r'^(?:void|size_t|bool|void\s*\*)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*;', line)
        if proto_match:
            func_name = proto_match.group(1)
            params_raw = proto_match.group(2).strip()

            j = i - 1
            comment_lines = []
            in_comment = False
            while j >= 0:
                prev_line = lines[j].strip()
                if prev_line.endswith("*/"):
                    in_comment = True
                if in_comment:
                    comment_lines.insert(0, prev_line)
                    if prev_line.startswith("/**") or prev_line.startswith("/*"):
                        break
                elif prev_line and not prev_line.startswith("//") and not prev_line.startswith("/*"):
                    break
                j -= 1

            comment_block = "\n".join(comment_lines)

            if not comment_block or not ("/**" in comment_block or "/*" in comment_block):
                print(f"ERROR: {header_path}:{i+1}: Function '{func_name}' is missing a Doxygen docstring comment.")
                errors += 1
            else:
                if "@brief" not in comment_block and not re.search(r"\*\s+[A-Z]", comment_block):
                    print(f"ERROR: {header_path}:{i+1}: Docstring for '{func_name}' lacks a @brief tag or description.")
                    errors += 1

                if params_raw and params_raw != "void":
                    param_list = [p.strip().split()[-1].lstrip("*") for p in params_raw.split(",") if p.strip()]
                    for p_name in param_list:
                        if not re.search(r"@param\s+(?:\[[^\]]+\]\s+)?" + re.escape(p_name) + r"\b", comment_block):
                            print(f"ERROR: {header_path}:{i+1}: Docstring for '{func_name}' missing '@param {p_name}'.")
                            errors += 1

                if not line.startswith("void ") and not line.startswith("void\t"):
                    if "@return" not in comment_block and "@returns" not in comment_block:
                        print(f"ERROR: {header_path}:{i+1}: Docstring for '{func_name}' missing '@return' tag.")
                        errors += 1

        i += 1

    if errors == 0:
        print(f"  ✓ All public function declarations in {os.path.basename(header_path)} have complete Doxygen docstrings.")

    return errors


def main():
    target_files = [
        os.path.join("vendor", "bootlib", "bootlib.h"),
        os.path.join("vendor", "bootlib", "bootlib.c"),
    ]

    total_errors = 0
    total_errors += run_clang_documentation_check(target_files)
    total_errors += lint_header_docstrings(target_files[0])

    if total_errors > 0:
        print(f"\n❌ Docstring linting failed with {total_errors} error(s).")
        sys.exit(1)

    print("\n✅ Docstring linting passed! All docstrings are present and correctly formatted.")
    sys.exit(0)

if __name__ == "__main__":
    main()
