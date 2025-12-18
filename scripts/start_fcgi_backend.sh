#!/usr/bin/env bash
set -euo pipefail

PORT="${2:-9000}"
CMD="${1:-start}"
LOG="/tmp/fcgi_backend_${PORT}.log"
PIDFILE="/tmp/fcgi_backend_${PORT}.pid"
WRAPPER="/tmp/fcgi_backend_${PORT}.py"

function check_python() {
    command -v python3 >/dev/null || { echo "python3 not found" >&2; exit 1; }
}

function ensure_flup() {
    python3 -c "import flup" 2>/dev/null && return 0
    echo "flup not found in python environment. Trying to install for user..." >&2
    command -v pip3 >/dev/null || { echo "pip3 not found; install flup manually" >&2; exit 1; }
    pip3 install --user flup6 >/dev/null 2>&1 || pip3 install --user flup >/dev/null 2>&1 || { echo "failed to install flup via pip" >&2; exit 1; }
}

function write_wrapper() {
cat > "$WRAPPER" <<'PY'
#!/usr/bin/env python3
# simple WSGI adapter that executes the script referenced by SCRIPT_FILENAME
from flup.server.fcgi import WSGIServer
import os, subprocess, sys


def app(env, start_response):
    script = env.get('SCRIPT_FILENAME') or env.get('PATH_TRANSLATED') or ''
    if not script:
        start_response('404 Not Found', [('Content-Type','text/plain')])
        return [b'No script specified']
    if not os.path.isfile(script):
        start_response('404 Not Found', [('Content-Type','text/plain')])
        return [b'Script not found']

    # Prepare environment for the script
    env_copy = {k: v for k, v in env.items() if isinstance(v, str)}
    # Read request body
    try:
        length = int(env.get('CONTENT_LENGTH', '0') or 0)
    except Exception:
        length = 0
    wsgi_input = env.get('wsgi.input')
    body = b''
    if wsgi_input and length > 0:
        body = wsgi_input.read(length)

    # Run the script as subprocess and capture output
    proc = subprocess.Popen(['python3', script], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env={**os.environ, **env_copy})
    stdout, stderr = proc.communicate(input=body)
    output = stdout.decode('utf-8', errors='replace')

    # Split headers/body
    headers = []
    status = '200 OK'
    if '\r\n\r\n' in output:
        hdrs, body = output.split('\r\n\r\n', 1)
    elif '\n\n' in output:
        hdrs, body = output.split('\n\n', 1)
    else:
        hdrs = ''
        body = output

    for line in hdrs.splitlines():
        if not line.strip():
            continue
        if line.lower().startswith('status:'):
            status = line.split(':',1)[1].strip()
        elif ':' in line:
            k, v = line.split(':',1)
            headers.append((k.strip(), v.strip()))

    if not any(h[0].lower() == 'content-type' for h in headers):
        headers.append(('Content-Type','text/plain'))

    start_response(status, headers)
    return [body.encode('utf-8')]


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9000
    WSGIServer(app, bindAddress=('127.0.0.1', port)).run()
PY
chmod +x "$WRAPPER"
}

case "$CMD" in
  start)
    check_python
    ensure_flup
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
      echo "FastCGI backend already running (pid $(cat "$PIDFILE"))"
      exit 0
    fi
    write_wrapper
    nohup python3 "$WRAPPER" "$PORT" > "$LOG" 2>&1 &
    echo $! > "$PIDFILE"
    echo "Started FastCGI backend on port $PORT (pid $(cat "$PIDFILE")), log: $LOG"
    ;;
  stop)
    if [ -f "$PIDFILE" ]; then
      PID="$(cat "$PIDFILE")"
      kill "$PID" 2>/dev/null || true
      rm -f "$PIDFILE" "$WRAPPER"
      echo "Stopped FastCGI backend (pid $PID)"
    else
      echo "No pidfile found ($PIDFILE)" >&2
      exit 1
    fi
    ;;
  status)
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
      echo "FastCGI backend running (pid $(cat "$PIDFILE"))"
    else
      echo "Not running" >&2
      exit 1
    fi
    ;;
  *)
    echo "Usage: $0 {start|stop|status} [port]" >&2
    exit 1
    ;;
esac
