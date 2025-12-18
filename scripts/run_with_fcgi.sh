#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-9000}"
CONFIG="${2:-config/fastcgi_test.conf}"

SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
BACKEND="$SCRIPTDIR/start_fcgi_backend.sh"

# Start backend
"$BACKEND" start "$PORT"
# Ensure it's stopped on exit
trap "$BACKEND stop $PORT" EXIT

echo "Starting webserv with config: $CONFIG (FastCGI backend on $PORT)"
./webserv "$CONFIG"
