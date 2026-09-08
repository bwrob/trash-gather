#!/usr/bin/env python3
"""
Docstring Linter for C/C++ codebase headers, implementations, and benchmark files.
Validates presence, completeness, and Doxygen formatting of docstrings.
"""

import argparse
import os
import re
import sys
import subprocess

def run_clang_documentation_check(c_files, cpp_files):
    """Run clang/clang++ with -Wdocumentation flags to catch Doxygen syntax/command errors."""
    errors = 0

    if c_files:
        print("=== Running Clang -Wdocumentation Linting (C) ===")
        cmd_c = [
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
        ] + c_files

        res_c = subprocess.run(cmd_c, capture_output=True, text=True)
        if res_c.returncode != 0 or res_c.stderr:
            doc_warnings = [line for line in res_c.stderr.splitlines() if "warning:" in line or "error:" in line]
            if doc_warnings:
                print("\n".join(doc_warnings))
                errors += len(doc_warnings)

        if errors == 0:
            print("  ✓ Clang C -Wdocumentation syntax checks passed cleanly.")

    if cpp_files:
        print("=== Running Clang++ -Wdocumentation Linting (C++) ===")
        bench_inc = "/opt/homebrew/opt/google-benchmark/include"
        extra_inc = ["-I" + bench_inc] if os.path.isdir(bench_inc) else []

        cmd_cpp = [
            "clang++",
            "-fsyntax-only",
            "-Wdocumentation",
            "-Wdocumentation-unknown-command",
            "-Wdocumentation-pedantic",
            "-std=c++17",
            "-Iinclude",
            "-Isrc",
            "-Ivendor/bootlib",
        ] + extra_inc + cpp_files

        res_cpp = subprocess.run(cmd_cpp, capture_output=True, text=True)
        if res_cpp.returncode != 0 or res_cpp.stderr:
            doc_warnings = [line for line in res_cpp.stderr.splitlines() if "warning:" in line or "error:" in line]
            if doc_warnings:
                print("\n".join(doc_warnings))
                errors += len(doc_warnings)

        if errors == 0:
            print("  ✓ Clang++ C++ -Wdocumentation syntax checks passed cleanly.")

    return errors


def lint_file_docstrings(file_path):
    """Verify presence and tag completeness (@brief, @param, @return) for declarations/definitions."""
    print(f"\n=== Linting Doxygen Docstrings in {os.path.basename(file_path)} ===")
    errors = 0

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Check for @file top-level docblock
    if not re.search(r"/\*\*.*?@file\b.*?\*/", content, re.DOTALL):
        print(f"ERROR: {file_path}: Missing top-level @file docstring block.")
        errors += 1

    # 2. Extract function declarations/definitions and preceding comments
    lines = content.splitlines()
    i = 0
    in_macro = False
    while i < len(lines):
        raw_line = lines[i]
        line = raw_line.strip()

        if in_macro:
            if not raw_line.endswith("\\"):
                in_macro = False
            i += 1
            continue

        if raw_line.startswith("#define") and raw_line.endswith("\\"):
            in_macro = True
            i += 1
            continue

        # Prototype or function definition matcher
        proto_match = re.match(
            r'^(?!typedef\b)(?:static\s+)?(?:const\s+)?(?:[a-zA-Z0-9_]+\s+\*?|\*[a-zA-Z0-9_]+\s+)([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*(?:;|\{)',
            line
        )
        if proto_match:
            func_name = proto_match.group(1)
            params_raw = proto_match.group(2).strip()

            # Ignore main entrypoints and macro constructs like BENCHMARK(...)
            if func_name not in ("main", "BENCHMARK", "BENCHMARK_MAIN"):
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
                    print(f"ERROR: {file_path}:{i+1}: Function '{func_name}' is missing a Doxygen docstring comment.")
                    errors += 1
                else:
                    if "@brief" not in comment_block and not re.search(r"\*\s+[A-Z]", comment_block):
                        print(f"ERROR: {file_path}:{i+1}: Docstring for '{func_name}' lacks a @brief tag or description.")
                        errors += 1

                    if params_raw and params_raw != "void":
                        param_list = [p.strip().split()[-1].lstrip("*&") for p in params_raw.split(",") if p.strip()]
                        for p_name in param_list:
                            if not re.search(r"@param\s+(?:\[[^\]]+\]\s+)?" + re.escape(p_name) + r"\b", comment_block):
                                print(f"ERROR: {file_path}:{i+1}: Docstring for '{func_name}' missing '@param {p_name}'.")
                                errors += 1

                    if not line.startswith("void ") and not line.startswith("static void ") and not line.startswith("void\t"):
                        if "@return" not in comment_block and "@returns" not in comment_block:
                            print(f"ERROR: {file_path}:{i+1}: Docstring for '{func_name}' missing '@return' tag.")
                            errors += 1

        i += 1

    if errors == 0:
        print(f"  ✓ All declarations/functions in {os.path.basename(file_path)} have complete Doxygen docstrings.")

    return errors


def discover_files(input_paths):
    """Discover C/C++ header, source, and benchmark files from input paths."""
    c_files = []
    cpp_files = []

    for path in input_paths:
        if os.path.isfile(path):
            if path.endswith((".h", ".c")):
                c_files.append(path)
            elif path.endswith((".hpp", ".cpp", ".cc", ".cxx")):
                cpp_files.append(path)
        elif os.path.isdir(path):
            for root, _, files in os.walk(path):
                for f in sorted(files):
                    file_path = os.path.join(root, f)
                    if f.endswith((".h", ".c")):
                        c_files.append(file_path)
                    elif f.endswith((".hpp", ".cpp", ".cc", ".cxx")):
                        cpp_files.append(file_path)
        else:
            print(f"WARNING: Path '{path}' does not exist or is not a file/directory.")

    return sorted(c_files), sorted(cpp_files)


def main():
    parser = argparse.ArgumentParser(description="Lint C/C++ docstrings across specified directories and files.")
    parser.add_argument(
        "paths",
        nargs="+",
        help="One or more file or directory paths to lint (e.g. vendor/bootlib bench)",
    )
    args = parser.parse_args()

    c_files, cpp_files = discover_files(args.paths)
    all_files = c_files + cpp_files

    if not all_files:
        print(f"No C/C++ header or source files found in paths: {args.paths}")
        sys.exit(0)

    total_errors = 0
    total_errors += run_clang_documentation_check(c_files, cpp_files)

    for f in all_files:
        total_errors += lint_file_docstrings(f)

    if total_errors > 0:
        print(f"\n❌ Docstring linting failed with {total_errors} error(s).")
        sys.exit(1)

    print("\n✅ Docstring linting passed! All docstrings are present and correctly formatted.")
    sys.exit(0)

if __name__ == "__main__":
    main()
