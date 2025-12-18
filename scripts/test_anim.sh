#!/usr/bin/env bash
set -euo pipefail

HOST=${1:-127.0.0.1}
PORT=${2:-8080}
ENDPOINT="http://$HOST:$PORT/cgi-bin/anim_frame.py"
COUNT=${3:-5}

echo "Fetching $COUNT frames from $ENDPOINT"
for i in $(seq 1 $COUNT); do
  echo "--- frame #$i ---"
  curl -sS "$ENDPOINT" | jq . || true
  sleep 0.15
done

echo "Open http://$HOST:$PORT/cgi-bin/anim.py in your browser to see the live animation."