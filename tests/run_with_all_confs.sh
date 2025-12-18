#!/usr/bin/env bash
set -eo pipefail

CONFIG_DIR="$(dirname "$0")/../config"
if [ ! -d "$CONFIG_DIR" ]; then
    echo "Config dir not found: $CONFIG_DIR" >&2
    exit 1
fi

echo "Running webserv against all configs in: $CONFIG_DIR"
printf "%-40s  %s\n" "CONFIG FILE" "RESULT"
printf '%s
' "$(printf '%.0s-' {1..70})"

results=()

for conf in "$CONFIG_DIR"/*; do
    [ -f "$conf" ] || continue
    fname=$(basename "$conf")
    log=$(mktemp /tmp/webserv-log-XXXXXX)

    # start webserv with this config in background
    ../webserv "$conf" &> "$log" &
    pid=$!

    # give it a short time to initialize or fail
    sleep 0.5

    if kill -0 "$pid" 2>/dev/null; then
        # still running -> we consider it started successfully
        # Capture any notable startup messages
        reason="started (pid $pid)"
        # shut it down gracefully
        kill "$pid" 2>/dev/null || true
        # wait for it to exit, with timeout
        for i in {1..10}; do
            if kill -0 "$pid" 2>/dev/null; then
                sleep 0.1
            else
                break
            fi
        done
        if kill -0 "$pid" 2>/dev/null; then
            kill -9 "$pid" 2>/dev/null || true
        fi
        printf "%-40s  OK    %s\n" "$fname" "$reason"
        results+=("$fname:OK:$reason:$log")
    else
        # process died quickly: show a brief error snippet
        errline=$(grep -i -m1 -E "error|unknown|fail|exception|throw" "$log" || true)
        if [ -z "$errline" ]; then
            errline=$(tail -n 10 "$log" | sed -n '1,3p')
        fi
        printf "%-40s  FAIL  %s\n" "$fname" "$(echo "$errline" | tr -d '\n')"
        results+=("$fname:FAIL:$(echo "$errline" | tr -d '\n'):$log")
    fi
done

printf '%s
' "$(printf '%.0s-' {1..70})"

# brief summary
ok_count=0
fail_count=0
for r in "${results[@]}"; do
    IFS=':' read -r name status _log <<< "$r"
    if [ "$status" = "OK" ]; then
        ok_count=$((ok_count+1))
    else
        fail_count=$((fail_count+1))
    fi
done

echo "Summary: $ok_count OK, $fail_count FAIL"
if [ $fail_count -gt 0 ]; then
    echo
    echo "Failed config files (short reasons):"
    for r in "${results[@]}"; do
        IFS=':' read -r name status reason logf <<< "$r"
        if [ "$status" = "FAIL" ]; then
            printf " - %s : %s\n" "$name" "$reason"
        fi
    done
    echo
    echo "Full logs are stored in /tmp/webserv-log-* (see the printed names above per file)"
fi

exit 0
