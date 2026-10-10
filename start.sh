#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

mkdir -p build
if [ ! -f "build/CMakeCache.txt" ]; then
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >&2
fi

cmake --build build --target server -j2 >&2 || cmake --build build -j2 >&2

SERVER_BIN="./build/server"
if [ ! -f "$SERVER_BIN" ] && [ -f "./build/server.exe" ]; then
    SERVER_BIN="./build/server.exe"
elif [ ! -f "$SERVER_BIN" ] && [ -f "./build/Release/server.exe" ]; then
    SERVER_BIN="./build/Release/server.exe"
elif [ ! -f "$SERVER_BIN" ] && [ -f "./build/cricpulse_server" ]; then
    SERVER_BIN="./build/cricpulse_server"
elif [ ! -f "$SERVER_BIN" ] && [ -f "./build/cricpulse_server.exe" ]; then
    SERVER_BIN="./build/cricpulse_server.exe"
fi

PORT="${PORT:-8080}"
PORT="$PORT" "$SERVER_BIN" &
PID=$!

cleanup() {
    if [ -n "$PID" ]; then
        kill -TERM "$PID" 2>/dev/null || true
        wait "$PID" 2>/dev/null || true
    fi
    exit 0
}

trap cleanup SIGTERM SIGINT EXIT

cpp_fingerprint() {
    if command -v sha256sum >/dev/null 2>&1; then
        find src include CMakeLists.txt -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name 'CMakeLists.txt' \) -exec sha256sum {} + 2>/dev/null | sort | sha256sum | awk '{print $1}'
    elif command -v md5sum >/dev/null 2>&1; then
        find src include CMakeLists.txt -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name 'CMakeLists.txt' \) -exec md5sum {} + 2>/dev/null | sort | md5sum | awk '{print $1}'
    else
        find src include CMakeLists.txt -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name 'CMakeLists.txt' \) -exec ls -l --full-time {} + 2>/dev/null | sort
    fi
}

LAST_CPP="$(cpp_fingerprint)"

while true; do
    sleep 1
    CURRENT_CPP="$(cpp_fingerprint)"
    if [ "$CURRENT_CPP" != "$LAST_CPP" ]; then
        echo "[CricPulse Watcher] Source changes detected. Rebuilding..." >&2
        if cmake --build build --target server -j2 >&2; then
            echo "[CricPulse Watcher] Build succeeded. Restarting server..." >&2
            kill -TERM "$PID" 2>/dev/null || true
            wait "$PID" 2>/dev/null || true
            sleep 0.5
            PORT="$PORT" "$SERVER_BIN" &
            PID=$!
            echo "[CricPulse Watcher] Server restarted successfully." >&2
            LAST_CPP="$CURRENT_CPP"
        else
            echo "[CricPulse Watcher] Build failed. Keeping existing server running." >&2
        fi
    fi
done
