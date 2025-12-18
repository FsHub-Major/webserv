#!/usr/bin/env bash
set -euo pipefail

PORT=${1:-8080}
HOST=${2:-127.0.0.1}
ENDPOINT="http://$HOST:$PORT/cgi-bin/dynamic.py"
COUNT=${3:-5}
DELAY=${4:-0.5}

echo "Testing dynamic CGI endpoint: $ENDPOINT (requests: $COUNT, delay: ${DELAY}s)"
for i in $(seq 1 $COUNT); do
    echo "--- request #$i ---"
    curl -sS -i "$ENDPOINT" | sed -n '1,8p'
    echo
    sleep "$DELAY"
done

echo "Done. Full counter file at /tmp/webserv_dynamic_counter contains:"; cat /tmp/webserv_dynamic_counter || true
