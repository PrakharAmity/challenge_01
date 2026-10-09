#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2

SERVER_BIN="./build/cricpulse_server"
if [ ! -f "$SERVER_BIN" ] && [ -f "./build/cricpulse_server.exe" ]; then
    SERVER_BIN="./build/cricpulse_server.exe"
fi

PORT="${PORT:-5000}" "$SERVER_BIN" & PID=$!
trap 'kill -TERM "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true; exit 0' SIGTERM SIGINT EXIT
snapshot(){ find src include -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -exec stat -c '%Y %n' {} + 2>/dev/null | sort; }
LAST="$(snapshot)"
while true; do
    sleep 2
    CURRENT="$(snapshot)"
    if [ "$CURRENT" != "$LAST" ]; then
        echo '[Engine] Rebuilding CricPulse...'
        if cmake --build build -j2; then
            kill -TERM "$PID" 2>/dev/null || true
            wait "$PID" 2>/dev/null || true
            PORT="${PORT:-3000}" "$SERVER_BIN" & PID=$!
            echo '[Engine] Server restarted.'
        fi
        LAST="$CURRENT"
    fi
done
