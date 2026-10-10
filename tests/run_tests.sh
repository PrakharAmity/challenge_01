#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

if [ ! -d "build" ]; then
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
fi

cmake --build build --target challenge_tests -j2 >/dev/null 2>&1 || cmake --build build -j2 >/dev/null 2>&1

if [ -f "./build/challenge_tests" ]; then
    exec ./build/challenge_tests
elif [ -f "./build/challenge_tests.exe" ]; then
    exec ./build/challenge_tests.exe
elif [ -f "./build/Release/challenge_tests.exe" ]; then
    exec ./build/Release/challenge_tests.exe
else
    echo "Test runner not found" >&2
    exit 1
fi
