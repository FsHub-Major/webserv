#!/usr/bin/env bash
set -euo pipefail

# Wrapper to run stress test with optional ulimit increase and a safe quick-precheck.
CLIENTS=${1:-15000}
HOST=${2:-127.0.0.1}
PORT=${3:-8080}
RAMP=${4:-30}

echo "Note: This script does NOT change kernel settings. You may need to run 'ulimit -n 65536' before running large tests."
if [ "$CLIENTS" -gt 2000 ]; then
  echo "You're about to launch $CLIENTS connections. Confirm (y/N)?"
  read -r ans
  if [ "$ans" != "y" ]; then
    echo "Aborted."; exit 1
  fi
fi

# Quick check: show current limit
echo "Current RLIMIT_NOFILE: $(ulimit -n)"

python3 scripts/stress_test.py --host "$HOST" --port "$PORT" --clients "$CLIENTS" --ramp "$RAMP" --concurrency 2000
