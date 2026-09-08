default: test

CC := "gcc"
CFLAGS := "-Wall -Wextra -std=c99 -g -fsanitize=address,undefined -Iinclude -Isrc -Ivendor/munit -Ivendor/bootlib"
COV_FLAGS := "-Wall -Wextra -std=c99 -g -fsanitize=address,undefined --coverage -Iinclude -Isrc -Ivendor/munit -Ivendor/bootlib"
BIN_DIR := "bin"

# Build all binaries
all: test build

# Create build output directory
[private]
mkdir-bin:
    @mkdir -p {{BIN_DIR}}

# Compile munit object
[private]
munit-obj: mkdir-bin
    {{CC}} {{CFLAGS}} -c vendor/munit/munit.c -o {{BIN_DIR}}/munit.o

# Compile src objects
[private]
src-objs: mkdir-bin
    {{CC}} {{CFLAGS}} -include bootlib.h -c vendor/bootlib/bootlib.c -o {{BIN_DIR}}/bootlib.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/sneknew.c -o {{BIN_DIR}}/sneknew.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/snekobject.c -o {{BIN_DIR}}/snekobject.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/stack.c -o {{BIN_DIR}}/stack.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/vm.c -o {{BIN_DIR}}/vm.o

# Compile and run unit tests
test: src-objs munit-obj
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_vm.c -o {{BIN_DIR}}/test_vm.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_mark.c -o {{BIN_DIR}}/test_mark.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_trace.c -o {{BIN_DIR}}/test_trace.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_snekobject.c -o {{BIN_DIR}}/test_snekobject.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_frame.c -o {{BIN_DIR}}/test_frame.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_sneknew.c -o {{BIN_DIR}}/test_sneknew.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_stack.c -o {{BIN_DIR}}/test_stack.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_runner.c -o {{BIN_DIR}}/test_runner.o
    {{CC}} {{CFLAGS}} {{BIN_DIR}}/bootlib.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/stack.o {{BIN_DIR}}/vm.o {{BIN_DIR}}/munit.o {{BIN_DIR}}/test_vm.o {{BIN_DIR}}/test_mark.o {{BIN_DIR}}/test_trace.o {{BIN_DIR}}/test_snekobject.o {{BIN_DIR}}/test_frame.o {{BIN_DIR}}/test_sneknew.o {{BIN_DIR}}/test_stack.o {{BIN_DIR}}/test_runner.o -o {{BIN_DIR}}/test_runner
    ./{{BIN_DIR}}/test_runner

# Measure line coverage using gcov / llvm-cov
coverage: mkdir-bin
    @rm -f {{BIN_DIR}}/*.gcda {{BIN_DIR}}/*.gcno
    {{CC}} {{CFLAGS}} -c vendor/munit/munit.c -o {{BIN_DIR}}/munit.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c vendor/bootlib/bootlib.c -o {{BIN_DIR}}/bootlib.o

    {{CC}} {{COV_FLAGS}} -include bootlib.h -c src/sneknew.c -o {{BIN_DIR}}/sneknew.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c src/snekobject.c -o {{BIN_DIR}}/snekobject.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c src/stack.c -o {{BIN_DIR}}/stack.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c src/vm.c -o {{BIN_DIR}}/vm.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_vm.c -o {{BIN_DIR}}/test_vm.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_mark.c -o {{BIN_DIR}}/test_mark.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_trace.c -o {{BIN_DIR}}/test_trace.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_snekobject.c -o {{BIN_DIR}}/test_snekobject.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_frame.c -o {{BIN_DIR}}/test_frame.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_sneknew.c -o {{BIN_DIR}}/test_sneknew.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_stack.c -o {{BIN_DIR}}/test_stack.o
    {{CC}} {{COV_FLAGS}} -include bootlib.h -c tests/test_runner.c -o {{BIN_DIR}}/test_runner.o
    {{CC}} {{COV_FLAGS}} {{BIN_DIR}}/bootlib.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/stack.o {{BIN_DIR}}/vm.o {{BIN_DIR}}/munit.o {{BIN_DIR}}/test_vm.o {{BIN_DIR}}/test_mark.o {{BIN_DIR}}/test_trace.o {{BIN_DIR}}/test_snekobject.o {{BIN_DIR}}/test_frame.o {{BIN_DIR}}/test_sneknew.o {{BIN_DIR}}/test_stack.o {{BIN_DIR}}/test_runner.o -o {{BIN_DIR}}/cov_runner
    ./{{BIN_DIR}}/cov_runner > /dev/null
    @if command -v xcrun >/dev/null 2>&1; then \
        xcrun llvm-cov gcov {{BIN_DIR}}/vm.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/stack.o; \
    else \
        gcov {{BIN_DIR}}/vm.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/stack.o; \
    fi


# Build the main sandbox executable
build: src-objs
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/main.c -o {{BIN_DIR}}/main.o
    {{CC}} {{CFLAGS}} {{BIN_DIR}}/bootlib.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/stack.o {{BIN_DIR}}/vm.o {{BIN_DIR}}/main.o -o {{BIN_DIR}}/main_app

# Run the main sandbox executable
run: build
    ./{{BIN_DIR}}/main_app

# Remove build artifacts
clean:
    rm -rf {{BIN_DIR}} *.gcov

# Format all C source and header files using clang-format
format:
    find src tests -type f -name '*.[ch]' | xargs clang-format -i

# Check formatting without modifying files
format-check:
    find src tests -type f -name '*.[ch]' | xargs clang-format --dry-run --Werror


# Run static analysis using clang-tidy (strictly on src/ files, ignoring vendor and tests)
lint:
    clang-tidy src/*.c -- -std=c99 -Iinclude -Isrc -Ivendor/munit -Ivendor/bootlib -include bootlib.h


# Install development dependencies via Homebrew Brewfile (macOS)
install-deps:
    brew bundle

BENCH_CXX := "clang++"
BENCH_FLAGS := "-O3 -std=c++17 -fsanitize=address,undefined -Iinclude -Isrc -Ivendor/bootlib -I/opt/homebrew/opt/google-benchmark/include"
BENCH_LIBS := "-L/opt/homebrew/opt/google-benchmark/lib -lbenchmark -pthread"


# Compile src objects for benchmarks (without bootlib override)
[private]
bench-objs: mkdir-bin
    {{CC}} {{CFLAGS}} -DBOOTLIB_NO_OVERRIDE -c vendor/bootlib/bootlib.c -o {{BIN_DIR}}/bench_bootlib.o
    {{CC}} {{CFLAGS}} -DBOOTLIB_NO_OVERRIDE -c src/sneknew.c -o {{BIN_DIR}}/bench_sneknew.o
    {{CC}} {{CFLAGS}} -DBOOTLIB_NO_OVERRIDE -c src/snekobject.c -o {{BIN_DIR}}/bench_snekobject.o
    {{CC}} {{CFLAGS}} -DBOOTLIB_NO_OVERRIDE -c src/stack.c -o {{BIN_DIR}}/bench_stack.o
    {{CC}} {{CFLAGS}} -DBOOTLIB_NO_OVERRIDE -c src/vm.c -o {{BIN_DIR}}/bench_vm.o

# Compile and run Google Benchmark performance benchmarks
bench: bench-objs
    {{BENCH_CXX}} {{BENCH_FLAGS}} bench/bench_gc.cpp {{BIN_DIR}}/bench_bootlib.o {{BIN_DIR}}/bench_sneknew.o {{BIN_DIR}}/bench_snekobject.o {{BIN_DIR}}/bench_stack.o {{BIN_DIR}}/bench_vm.o {{BENCH_LIBS}} -o {{BIN_DIR}}/bench_runner
    ./{{BIN_DIR}}/bench_runner

# Install local Git pre-commit hook (format-check & test verification)
setup-hooks:
    @mkdir -p .git/hooks
    @echo '#!/bin/sh' > .git/hooks/pre-commit
    @echo 'echo "=== [Pre-commit Hook] Verifying format and running tests ==="' >> .git/hooks/pre-commit
    @echo 'just format-check || { echo "[Pre-commit Error] Formatting check failed! Run '\''just format'\'' to fix."; exit 1; }' >> .git/hooks/pre-commit
    @echo 'just test || { echo "[Pre-commit Error] Unit tests failed!"; exit 1; }' >> .git/hooks/pre-commit
    @chmod +x .git/hooks/pre-commit
    @echo "Git pre-commit hook successfully installed to .git/hooks/pre-commit!"








