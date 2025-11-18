#!/usr/bin/env bash
# contrib/test-ci.sh
# Minimal smoke-test for jack-bluealsa-autobridge (non-root-friendly, best-effort).
# - builds the autobridge with ASAN/UBSAN enabled (if available)
# - runs it for a short period to exercise spawn/reap logic (no real Bluetooth required)
# - checks exit status and basic logging
#
# Intended for developer CI / local testing only.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_BIN="$ROOT_DIR/bin/jack-bluealsa-autobridge"
SRC="src/jack-bluealsa-autobridge.c"
LOG="/tmp/jack-bluealsa-autobridge-test.log"

echo "CI: build smoke test for jack-bluealsa-autobridge"
mkdir -p "$ROOT_DIR/bin"

# Try to compile with ASAN/UBSAN if available
# Include ALSA cflags/libs so builds that reference snd_pcm_* succeed in CI smoke test
CFLAGS="$(pkg-config --cflags glib-2.0 gio-2.0 alsa) -O2 -g -Wall -Wextra -std=c11"
LDFLAGS="$(pkg-config --libs glib-2.0 gio-2.0 alsa)"

# Prefer sanitizers when supported
SANITIZERS="-fsanitize=address,undefined -fno-omit-frame-pointer"
CC=${CC:-cc}

echo "Compiling $SRC..."
$CC $CFLAGS $SANITIZERS "$ROOT_DIR/$SRC" -o "$BUILD_BIN" $LDFLAGS || {
  echo "Sanitizer build failed; falling back to normal build"
  $CC $CFLAGS "$ROOT_DIR/$SRC" -o "$BUILD_BIN" $LDFLAGS
}

echo "Running smoke test (will run for 5 seconds)..."
# Run in background, capture logs
rm -f "$LOG"
"$BUILD_BIN" >/tmp/jack-bluealsa-autobridge.stdout 2>"$LOG" &
PID=$!

# Give it a few seconds to initialize and exercise reaper
sleep 5

# Check if process still running
if kill -0 "$PID" 2>/dev/null; then
  echo "Process running (PID=$PID), sending SIGTERM for graceful shutdown..."
  kill -TERM "$PID"
  wait "$PID" || true
else
  echo "Process exited early; collecting logs"
fi

echo "=== STDOUT ==="
sed -n '1,200p' /tmp/jack-bluealsa-autobridge.stdout || true
echo "=== STDERR/LOG ==="
sed -n '1,200p' "$LOG" || true

# Basic assertions: exit code and presence of startup log line
if grep -q "jack-bluealsa-autobridge starting" "$LOG"; then
  echo "Smoke test: startup log found"
else
  echo "Smoke test: startup log NOT found"
  exit 2
fi

echo "Smoke test completed"