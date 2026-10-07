#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
cmake --build build --target challenge_tests -j2 >/dev/null 2>&1
exec ./build/challenge_tests
