#!/usr/bin/env python3
"""
Docstring Linter for C/C++ codebase headers, implementations, and benchmark files.
Validates presence, completeness, and Doxygen formatting of docstrings.
"""

import argparse
import os
import re
import subprocess
import sys

from project_config import get_c_standard, get_cpp_standard


def _run_clang_c(c_files: list[str]) -> int:
    """Run clang -Wdocumentation on C source and header files."""
    print("=== Running Clang -Wdocumentation Linting (C) ===")
    cmd_c = [
        "clang",
        "-fsyntax-only",
        "-Wdocumentation",
        "-Wdocumentation-unknown-command",
        "-Wdocumentation-pedantic",
        f"-std={get_c_standard()}",
        "-Iinclude",
        "-Isrc",
        "-Ivendor/munit",
        "-Ivendor/bootlib",
    ] + c_files

    try:
        res = subprocess.run(cmd_c, capture_output=True, text=True, check=False)
        warnings = [
            line for line in res.stderr.splitlines() if "warning:" in line or "error:" in line
        ]
        if warnings:
            print("\n".join(warnings))
            return len(warnings)
    except FileNotFoundError:
        print("WARNING: 'clang' not found in PATH; skipping Clang C -Wdocumentation check.")
        return 0

    print("  ✓ Clang C -Wdocumentation syntax checks passed cleanly.")
    return 0


def _run_clang_cpp(cpp_files: list[str]) -> int:
    """Run clang++ -Wdocumentation on C++ source and benchmark files."""
    print("=== Running Clang++ -Wdocumentation Linting (C++) ===")
    bench_paths = [
        "/opt/homebrew/opt/google-benchmark/include",
        "/usr/local/opt/google-benchmark/include",
        "/usr/include",
        "/usr/local/include",
    ]
    extra_inc = ["-I" + p for p in bench_paths if os.path.isdir(p)]

    cmd_cpp = (
        [
            "clang++",
            "-fsyntax-only",
            "-Wdocumentation",
            "-Wdocumentation-unknown-command",
            "-Wdocumentation-pedantic",
            f"-std={get_cpp_standard()}",
            "-Iinclude",
            "-Isrc",
            "-Ivendor/bootlib",
        ]
        + extra_inc
        + cpp_files
    )

    try:
        res = subprocess.run(cmd_cpp, capture_output=True, text=True, check=False)
        warnings = [
            line
            for line in res.stderr.splitlines()
            if ("warning:" in line or "error:" in line) and "file not found" not in line
        ]
        if warnings:
            print("\n".join(warnings))
            return len(warnings)
    except FileNotFoundError:
        print("WARNING: 'clang++' not found in PATH; skipping Clang++ -Wdocumentation check.")
        return 0

    print("  ✓ Clang++ C++ -Wdocumentation syntax checks passed cleanly.")
    return 0


def run_clang_documentation_check(c_files: list[str], cpp_files: list[str]) -> int:
    """Run clang/clang++ with -Wdocumentation flags to catch Doxygen syntax/command errors."""
    errors = 0
    if c_files:
        errors += _run_clang_c(c_files)
    if cpp_files:
        errors += _run_clang_cpp(cpp_files)
    return errors


def _check_file_docblock(file_path: str, content: str) -> int:
    """Verify presence of top-level @file Doxygen block."""
    if not re.search(r"/\*\*.*?@file\b.*?\*/", content, re.DOTALL):
        print(f"ERROR: {file_path}: Missing top-level @file docstring block.")
        return 1
    return 0


def _extract_preceding_comment(lines: list[str], start_idx: int) -> str:
    """Extract comment lines immediately preceding a function declaration."""
    comment_lines: list[str] = []
    in_comment = False
    j = start_idx - 1

    while j >= 0:
        prev_line = lines[j].strip()
        if prev_line.endswith("*/"):
            in_comment = True
        if in_comment:
            comment_lines.insert(0, prev_line)
            if prev_line.startswith(("/**", "/*")):
                break
        elif prev_line and not prev_line.startswith(("//", "/*")):
            break
        j -= 1

    return "\n".join(comment_lines)


def _check_docstring_params(
    file_path: str, line_no: int, func_name: str, params_raw: str, comment_block: str
) -> int:
    """Check @param tag presence for each parameter in a non-void function."""
    if not params_raw or params_raw == "void":
        return 0

    errors = 0
    param_list = [
        p.strip().split()[-1].lstrip("*&").rstrip("[]") for p in params_raw.split(",") if p.strip()
    ]
    for p_name in param_list:
        pattern = r"@param\s+(?:\[[^\]]+\]\s+)?" + re.escape(p_name) + r"\b"
        if not re.search(pattern, comment_block):
            msg = f"Docstring for '{func_name}' missing '@param {p_name}'."
            print(f"ERROR: {file_path}:{line_no}: {msg}")
            errors += 1
    return errors


def _check_docstring_return(
    file_path: str, line_no: int, func_name: str, line: str, comment_block: str
) -> int:
    """Check @return tag presence for non-void returning functions."""
    is_pure_void = bool(re.match(r"^(?:(?:static|inline|extern)\s+)*void(?:\s+|\t+)[^*]", line))
    if not is_pure_void and "@return" not in comment_block and "@returns" not in comment_block:
        print(f"ERROR: {file_path}:{line_no}: Docstring for '{func_name}' missing '@return' tag.")
        return 1
    return 0


def _validate_function_docstring(
    file_path: str,
    line_no: int,
    func_name: str,
    params_raw: str,
    line: str,
    is_munit: bool,
    comment_block: str,
) -> int:
    """Validate docblock tags (@brief, @param, @return) for a single function."""
    if not comment_block or ("/**" not in comment_block and "/*" not in comment_block):
        print(
            f"ERROR: {file_path}:{line_no}: "
            f"Function '{func_name}' is missing a Doxygen docstring comment."
        )
        return 1

    errors = 0
    if "@brief" not in comment_block and not re.search(r"\*\s+[A-Z]", comment_block):
        print(
            f"ERROR: {file_path}:{line_no}: "
            f"Docstring for '{func_name}' lacks a @brief tag or description."
        )
        errors += 1

    if not is_munit:
        errors += _check_docstring_params(file_path, line_no, func_name, params_raw, comment_block)
        errors += _check_docstring_return(file_path, line_no, func_name, line, comment_block)

    return errors


def _match_function_decl(line: str) -> tuple[str | None, str, bool]:
    """Parse a source line for function prototype or munit_case definition."""
    munit_match = re.match(r"^munit_case\s*\(\s*[A-Z_]+\s*,\s*([a-zA-Z0-9_]+)", line)
    if munit_match:
        return munit_match.group(1), "", True

    proto_match = re.match(
        r"^(?!typedef\b)(?!return\b)(?:static\s+)?(?:const\s+)?(?:[a-zA-Z0-9_]+\s+\*?|\*[a-zA-Z0-9_]+\s+)([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*(?:;|\{)",
        line,
    )
    if proto_match:
        return proto_match.group(1), proto_match.group(2).strip(), False

    return None, "", False


def _scan_functions(file_path: str, lines: list[str]) -> int:
    """Scan file lines for function declarations and validate their docstrings."""
    errors = 0
    in_macro = False

    for i, raw_line in enumerate(lines):
        if in_macro:
            if not raw_line.endswith("\\"):
                in_macro = False
            continue

        if raw_line.startswith("#define") and raw_line.endswith("\\"):
            in_macro = True
            continue

        func_name, params_raw, is_munit = _match_function_decl(raw_line.strip())
        if not func_name or func_name in ("main", "BENCHMARK", "BENCHMARK_MAIN"):
            continue

        comment_block = _extract_preceding_comment(lines, i)
        errors += _validate_function_docstring(
            file_path, i + 1, func_name, params_raw, raw_line.strip(), is_munit, comment_block
        )

    return errors


def lint_file_docstrings(file_path: str) -> int:
    """Verify presence and tag completeness (@brief, @param, @return)."""
    print(f"\n=== Linting Doxygen Docstrings in {os.path.basename(file_path)} ===")
    with open(file_path, encoding="utf-8") as f:
        content = f.read()

    errors = _check_file_docblock(file_path, content)
    errors += _scan_functions(file_path, content.splitlines())

    if errors == 0:
        base_name = os.path.basename(file_path)
        print(f"  ✓ All declarations/functions in {base_name} have complete Doxygen docstrings.")

    return errors


def _categorize_file(file_path: str, c_files: list[str], cpp_files: list[str]) -> None:
    """Sort path into c_files or cpp_files based on file extension."""
    if file_path.endswith((".h", ".c")):
        c_files.append(file_path)
    elif file_path.endswith((".hpp", ".cpp", ".cc", ".cxx")):
        cpp_files.append(file_path)


def _scan_directory(dir_path: str, c_files: list[str], cpp_files: list[str]) -> None:
    """Walk directory recursively and categorize all discovered source files."""
    for root, _, files in os.walk(dir_path):
        for f in sorted(files):
            _categorize_file(os.path.join(root, f), c_files, cpp_files)


def discover_files(input_paths: list[str]) -> tuple[list[str], list[str]]:
    """Discover C/C++ header, source, and benchmark files from input paths."""
    c_files: list[str] = []
    cpp_files: list[str] = []

    for path in input_paths:
        if os.path.isfile(path):
            _categorize_file(path, c_files, cpp_files)
        elif os.path.isdir(path):
            _scan_directory(path, c_files, cpp_files)
        else:
            print(f"WARNING: Path '{path}' does not exist or is not a file/directory.")

    return sorted(c_files), sorted(cpp_files)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Lint C/C++ docstrings across specified directories and files."
    )
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
