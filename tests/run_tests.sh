#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

if [ ! -d "build" ]; then
    if command -v ninja >/dev/null 2>&1; then
        cmake -B build -S . -G Ninja >/dev/null 2>&1 || cmake -B build -S . >/dev/null 2>&1
    else
        cmake -B build -S . >/dev/null 2>&1
    fi
fi

cmake --build build --target test_runner >/dev/null 2>&1 || cmake --build build >/dev/null 2>&1 || true

if [ -f "./build/test_runner" ]; then
    exec ./build/test_runner
elif [ -f "./build/test_runner.exe" ]; then
    exec ./build/test_runner.exe
elif [ -f "./build/Release/test_runner.exe" ]; then
    exec ./build/Release/test_runner.exe
elif [ -f "./build/challenge_tests" ]; then
    exec ./build/challenge_tests
elif [ -f "./build/challenge_tests.exe" ]; then
    exec ./build/challenge_tests.exe
else
    echo "Test runner not found" >&2
    exit 1
fi
