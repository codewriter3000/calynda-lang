#!/bin/bash
# run_tests.sh — Build and run all Calynda tests using GCC via Git Bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building tests ==="
make clean
make test

EXIT=$?
if [ $EXIT -eq 0 ]; then
    echo ""
    echo "All tests passed."
else
    echo ""
    echo "Some tests failed (exit code $EXIT)."
fi

exit $EXIT
