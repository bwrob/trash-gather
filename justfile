default: test

# ==============================================================================
# Global Configuration Variables (Single Source of Truth)
# ==============================================================================

# Dynamically discover all include directories in src/
SRC_INCS := `find src -type d | sort | sed 's|^|-I|' | tr '\n' ' '`

# Language Standards
export C_STD := "c17"
export CPP_STD := "c++17"

# C Compiler & Build Tooling
CC := "gcc"
CFLAGS := "-Wall -Wextra -Wswitch -std=" + C_STD + " -g -fsanitize=address,undefined -Iinclude " + SRC_INCS + "-Ivendor/munit -Ivendor/bootlib"
COV_FLAGS := CFLAGS + " --coverage"
BIN_DIR := "bin"

# Dynamically discover all runtime source modules in src/ (excluding main.c)
SRC_OBJS := `find src -name '*.c' ! -name 'main.c' -exec basename {} .c \; | sort | sed 's|^|bin/|;s|$|\.o|' | tr '\n' ' '`
BENCH_SRC_OBJS := `find src -name '*.c' ! -name 'main.c' -exec basename {} .c \; | sort | sed 's|^|bin/bench_|;s|$|\.o|' | tr '\n' ' '`
TEST_OBJS := `find tests -name 'test_*.c' ! -name 'test_runner.c' -exec basename {} .c \; | sort | sed 's|^|bin/|;s|$|\.o|' | tr '\n' ' '`

# Docstring Linting Scope (directories passed to scripts/lint_docstrings.py)
DOC_LINT_DIRS := "include vendor/bootlib bench tests"

# Benchmark Configuration (Google Benchmark / C++)
BENCH_CXX := "clang++"
BREW_BENCH_INC := `pkg-config --cflags-only-I benchmark 2>/dev/null || if [ -d /opt/homebrew/opt/google-benchmark/include ]; then echo "-I/opt/homebrew/opt/google-benchmark/include"; elif [ -d /usr/local/opt/google-benchmark/include ]; then echo "-I/usr/local/opt/google-benchmark/include"; fi`
BREW_BENCH_LIB := `pkg-config --libs benchmark 2>/dev/null || if [ -d /opt/homebrew/opt/google-benchmark/lib ]; then echo "-L/opt/homebrew/opt/google-benchmark/lib -lbenchmark -pthread"; else echo "-lbenchmark -pthread"; fi`
BENCH_FLAGS := "-O3 -std=" + CPP_STD + " -fsanitize=address,undefined -Iinclude " + SRC_INCS + "-Ivendor/bootlib " + BREW_BENCH_INC
BENCH_LIBS := BREW_BENCH_LIB

# ==============================================================================

# Build all binaries and generate compile_commands.json
all: test build compiledb

# Create build output directory
[private]
mkdir-bin:
    @mkdir -p {{ BIN_DIR }}

# Compile munit object
[private]
@munit-obj: mkdir-bin
    {{ CC }} {{ CFLAGS }} -c vendor/munit/munit.c -o {{ BIN_DIR }}/munit.o

# Compile src objects
[private]
@src-objs: mkdir-bin
    {{ CC }} {{ CFLAGS }} -include bootlib.h -c vendor/bootlib/bootlib.c -o {{ BIN_DIR }}/bootlib.o
    @set -e; for f in $(find src -name '*.c' ! -name 'main.c'); do \
        base=$(basename "$f" .c); \
        {{ CC }} {{ CFLAGS }} -include bootlib.h -c "$f" -o "{{ BIN_DIR }}/${base}.o"; \
    done

# Compile test runner binary
@test-build: src-objs munit-obj
    @set -e; for f in $(find tests -name '*.c'); do \
        base=$(basename "$f" .c); \
        {{ CC }} {{ CFLAGS }} -include bootlib.h -c "$f" -o "{{ BIN_DIR }}/${base}.o"; \
    done
    {{ CC }} {{ CFLAGS }} {{ BIN_DIR }}/bootlib.o {{ SRC_OBJS }} {{ BIN_DIR }}/munit.o {{ TEST_OBJS }} {{ BIN_DIR }}/test_runner.o -o {{ BIN_DIR }}/test_runner

# Run unit tests (only displaying errors, failures, and summary)
@test *args="": test-build
    uv run python scripts/run_tests.py {{ args }}

# Run unit tests in verbose mode (displaying all passing and failing tests)
test-verbose *args="": test-build
    ./{{ BIN_DIR }}/test_runner {{ args }}

# List all available unit tests
test-list: test-build
    ./{{ BIN_DIR }}/test_runner --list

# Run tests matching a specific pattern or prefix (e.g. `just test-filter trace` or `just test-filter stack`)
@test-filter pattern: test-build
    @TESTS=$(./{{ BIN_DIR }}/test_runner --list | grep "{{ pattern }}"); \
    if [ -z "$TESTS" ]; then \
        echo "No tests matched pattern: '{{ pattern }}'"; exit 1; \
    else \
        uv run python scripts/run_tests.py $TESTS; \
    fi

# Run interactive LLDB debugger on test suite with --no-fork (or on matching test pattern)
debug filter="": test-build
    @if [ -z "{{ filter }}" ]; then \
        lldb -- ./{{ BIN_DIR }}/test_runner --no-fork; \
    else \
        TESTS=$(./{{ BIN_DIR }}/test_runner --list | grep "{{ filter }}"); \
        if [ -z "$TESTS" ]; then \
            echo "No tests matched pattern: '{{ filter }}'"; exit 1; \
        else \
            lldb -- ./{{ BIN_DIR }}/test_runner --no-fork $TESTS; \
        fi; \
    fi

# Continuous watch mode: auto-recompiles and tests on any .c/.h/.py file save
watch:
    watchexec -e c,h,cpp,py "just test"

# Inspect OS-level memory leaks on macOS
leaks: test-build
    leaks --atExit -- ./{{ BIN_DIR }}/test_runner

# Measure line coverage using gcov / llvm-cov
coverage: mkdir-bin
    @rm -f {{ BIN_DIR }}/*.gcda {{ BIN_DIR }}/*.gcno
    {{ CC }} {{ CFLAGS }} -c vendor/munit/munit.c -o {{ BIN_DIR }}/munit.o
    {{ CC }} {{ COV_FLAGS }} -include bootlib.h -c vendor/bootlib/bootlib.c -o {{ BIN_DIR }}/bootlib.o

    @set -e; for f in $(find src -name '*.c' ! -name 'main.c'); do \
        base=$(basename "$f" .c); \
        {{ CC }} {{ COV_FLAGS }} -include bootlib.h -c "$f" -o "{{ BIN_DIR }}/${base}.o"; \
    done
    @set -e; for f in $(find tests -name '*.c'); do \
        base=$(basename "$f" .c); \
        {{ CC }} {{ COV_FLAGS }} -include bootlib.h -c "$f" -o "{{ BIN_DIR }}/${base}.o"; \
    done
    {{ CC }} {{ COV_FLAGS }} {{ BIN_DIR }}/bootlib.o {{ SRC_OBJS }} {{ BIN_DIR }}/munit.o {{ TEST_OBJS }} {{ BIN_DIR }}/test_runner.o -o {{ BIN_DIR }}/cov_runner
    ./{{ BIN_DIR }}/cov_runner > /dev/null
    @if command -v xcrun >/dev/null 2>&1; then \
        xcrun llvm-cov gcov {{ SRC_OBJS }}; \
    else \
        gcov {{ SRC_OBJS }}; \
    fi
    @rm -f *.gcov

# Build the main sandbox executable
build: src-objs
    {{ CC }} {{ CFLAGS }} -include bootlib.h -c src/main.c -o {{ BIN_DIR }}/main.o
    {{ CC }} {{ CFLAGS }} {{ BIN_DIR }}/bootlib.o {{ SRC_OBJS }} {{ BIN_DIR }}/main.o -o {{ BIN_DIR }}/main_app

# Run the main sandbox executable
run: build
    ./{{ BIN_DIR }}/main_app

# Remove build artifacts and temporary files
clean:
    rm -rf {{ BIN_DIR }} *.gcov

# Generate compile_commands.json for clangd / language server intelligence
compiledb:
    uv run python scripts/gen_compile_commands.py

# Format all C/C++ source files, Markdown documentation, and TOML configurations
# Format all C/C++ source files, Markdown documentation, TOML configurations, and justfile
format: format-c format-md format-toml format-just

# Format all C/C++ source and header files using clang-format
format-c:
    find src tests bench include -type f \( -name '*.[ch]' -o -name '*.cpp' \) | xargs uv run clang-format -i

# Check C/C++ formatting without modifying files
format-c-check:
    find src tests bench include -type f \( -name '*.[ch]' -o -name '*.cpp' \) | xargs uv run clang-format --dry-run --Werror

# Format Markdown documentation and skills with rumdl
format-md:
    uv run rumdl fmt

# Check Markdown formatting without modifying files
format-md-check:
    uv run rumdl fmt --check

# Lint Markdown files with rumdl
lint-md:
    uv run rumdl check

# Format TOML configuration files using taplo
format-toml:
    uv run taplo format

# Check TOML formatting without modifying files
format-toml-check:
    uv run taplo format --check

# Format justfile using just --fmt
format-just:
    just --fmt

# Check justfile formatting without modifying files
format-just-check:
    just --fmt --check

# Check all formatting without modifying files
format-check: format-c-check format-md-check format-toml-check format-just-check

# Check Python code formatting, linting, types, and cognitive complexity (ruff, pyrefly, complexipy)
lint-py:
    uv run ruff check scripts/
    uv run ruff format --check scripts/
    uv run pyrefly check scripts/
    uv run complexipy

# Automatically format Python scripts and fix lint issues
format-py:
    uv run ruff format scripts/
    uv run ruff check --fix scripts/

# Run docstring linting across configured DOC_LINT_DIRS
lint-docs:
    uv run python scripts/lint_docstrings.py {{ DOC_LINT_DIRS }}

# Run static analysis on C source files using clang-tidy
lint-c:
    clang-tidy $(find src -name '*.c') -- -std={{ C_STD }} -Iinclude {{ SRC_INCS }} -Ivendor/munit -Ivendor/bootlib -include bootlib.h

# Update Mermaid DAG in roadmap/README.md with transitive reduction
update-dag:
    uv run python scripts/generate_roadmap_dag.py

# Generate Mermaid DAG of C header dependencies and save to docs/header_dag.mmd
header-dag:
    uv run python scripts/generate_header_dag.py --output docs/header_dag.mmd

# Synchronize header DAG in docs/header_dag.mmd and root README.md
update-header-dag: header-dag
    uv run python scripts/update_readme_header_dag.py

# Verify that C header dependencies form a strict DAG and README.md is in sync
lint-header-dag:
    uv run python scripts/generate_header_dag.py --check-dag > /dev/null
    uv run python scripts/update_readme_header_dag.py --check

# Verify that all C header files compile in total isolation without external dependencies
lint-headers-standalone:
    uv run python scripts/lint_headers_standalone.py

# Validate roadmap milestone hash IDs, DAG consistency, and markdown links
lint-roadmap:
    uv run python scripts/lint_roadmap.py

# Scaffold a new roadmap milestone writeup from template
new-milestone slug title="" difficulty="3":
    uv run python scripts/new_milestone.py {{ slug }} "{{ title }}" --difficulty {{ difficulty }}

# Scaffold a new workspace agent skill with templates and subdirectories
new-skill name desc="":
    uv run python .agents/skills/skill-creator/scripts/scaffold_skill.py {{ name }} {{ if desc != "" { "--description \"" + desc + "\"" } else { "" } }}

# Run static analysis and formatting checks (clang-tidy, docstrings, roadmap, Python, Markdown, and justfile)
lint: lint-c lint-docs lint-roadmap lint-py lint-md format-just-check

# Install development dependencies via Homebrew Brewfile (macOS)
install-deps:
    brew bundle

# Compile src objects for benchmarks (without bootlib override)
[private]
bench-objs: mkdir-bin
    {{ CC }} {{ CFLAGS }} -DBOOTLIB_NO_OVERRIDE -c vendor/bootlib/bootlib.c -o {{ BIN_DIR }}/bench_bootlib.o
    @set -e; for f in $(find src -name '*.c' ! -name 'main.c'); do \
        base=$(basename "$f" .c); \
        {{ CC }} {{ CFLAGS }} -DBOOTLIB_NO_OVERRIDE -c "$f" -o "{{ BIN_DIR }}/bench_${base}.o"; \
    done

# Compile benchmark runner binary
bench-build: bench-objs
    {{ BENCH_CXX }} {{ BENCH_FLAGS }} bench/bench_gc.cpp {{ BIN_DIR }}/bench_bootlib.o {{ BENCH_SRC_OBJS }} {{ BENCH_LIBS }} -o {{ BIN_DIR }}/bench_runner

# Compile and run Google Benchmark performance benchmarks
bench: bench-build
    ./{{ BIN_DIR }}/bench_runner

# Run all pre-commit hooks manually across all files
check:
    uv run pre-commit run --all-files

# Install Git pre-commit hook via pre-commit tool
setup-hooks:
    uv run pre-commit install
