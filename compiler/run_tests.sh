#!/bin/bash
# run_tests.sh — Build and run all Calynda tests
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building tests ==="
make clean
make test

EXIT=$?

echo ""
echo "=== Cross-compilation toolchain status ==="
if command -v aarch64-linux-gnu-gcc > /dev/null 2>&1; then
    echo "  aarch64-linux-gnu-gcc: found ($(aarch64-linux-gnu-gcc -dumpversion))"
    make runtime-aarch64 2>/dev/null && echo "  runtime-aarch64: built" || echo "  runtime-aarch64: build failed"
else
    echo "  aarch64-linux-gnu-gcc: not found (aarch64 cross tests skipped)"
fi

if command -v riscv64-linux-gnu-gcc > /dev/null 2>&1; then
    echo "  riscv64-linux-gnu-gcc: found ($(riscv64-linux-gnu-gcc -dumpversion))"
    make runtime-riscv64 2>/dev/null && echo "  runtime-riscv64: built" || echo "  runtime-riscv64: build failed"
else
    echo "  riscv64-linux-gnu-gcc: not found (riscv64 cross tests skipped)"
fi

if command -v qemu-riscv64 > /dev/null 2>&1; then
    echo "  qemu-riscv64: found ($(qemu-riscv64 --version 2>&1 | head -1))"
else
    echo "  qemu-riscv64: not found (riscv64 QEMU execution tests skipped)"
fi

if [ $EXIT -eq 0 ]; then
    echo ""
    echo "All tests passed."
else
    echo ""
    echo "Some tests failed (exit code $EXIT)."
fi

exit $EXIT
