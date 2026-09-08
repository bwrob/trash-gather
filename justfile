default: test

CC := "gcc"
CFLAGS := "-Wall -Wextra -std=c99 -g -fsanitize=address,undefined -Iinclude -Isrc -Ivendor/munit"
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
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/bootlib.c -o {{BIN_DIR}}/bootlib.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/sneknew.c -o {{BIN_DIR}}/sneknew.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/snekobject.c -o {{BIN_DIR}}/snekobject.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/stack.c -o {{BIN_DIR}}/stack.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/vm.c -o {{BIN_DIR}}/vm.o

# Compile and run unit tests
test: src-objs munit-obj
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_vm.c -o {{BIN_DIR}}/test_vm.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_trace.c -o {{BIN_DIR}}/test_trace.o
    {{CC}} {{CFLAGS}} -include bootlib.h -c tests/test_runner.c -o {{BIN_DIR}}/test_runner.o
    {{CC}} {{CFLAGS}} {{BIN_DIR}}/bootlib.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/stack.o {{BIN_DIR}}/vm.o {{BIN_DIR}}/munit.o {{BIN_DIR}}/test_vm.o {{BIN_DIR}}/test_trace.o {{BIN_DIR}}/test_runner.o -o {{BIN_DIR}}/test_runner
    ./{{BIN_DIR}}/test_runner

# Build the main sandbox executable
build: src-objs
    {{CC}} {{CFLAGS}} -include bootlib.h -c src/main.c -o {{BIN_DIR}}/main.o
    {{CC}} {{CFLAGS}} {{BIN_DIR}}/bootlib.o {{BIN_DIR}}/sneknew.o {{BIN_DIR}}/snekobject.o {{BIN_DIR}}/stack.o {{BIN_DIR}}/vm.o {{BIN_DIR}}/main.o -o {{BIN_DIR}}/main_app

# Run the main sandbox executable
run: build
    ./{{BIN_DIR}}/main_app

# Remove build artifacts
clean:
    rm -rf {{BIN_DIR}}
