#!/bin/bash
# FastCGI Test Runner Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "FastCGI Test Setup and Execution"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if server binary exists
if [ ! -f "$PROJECT_ROOT/webserv" ]; then
    echo -e "${RED}Error: webserv binary not found${NC}"
    echo "Run 'make' first to build the server"
    exit 1
fi

# Check if test scripts exist
if [ ! -f "$PROJECT_ROOT/cgi-bin/test_get.py" ]; then
    echo -e "${RED}Error: Test scripts not found in cgi-bin/${NC}"
    exit 1
fi

# Ensure scripts are executable
chmod +x "$PROJECT_ROOT"/cgi-bin/test_*.py

echo "Step 1: Checking for Python and flup..."
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 not found${NC}"
    exit 1
fi

if ! python3 -c "import flup" 2>/dev/null; then
    echo -e "${YELLOW}Warning: flup not installed${NC}"
    echo "Installing flup (FastCGI support for Python)..."
    pip3 install flup6 || pip3 install flup || {
        echo -e "${RED}Failed to install flup${NC}"
        echo "Try: pip3 install flup6"
        exit 1
    }
fi

echo -e "${GREEN}✓ Python and flup ready${NC}"
echo ""

echo "Step 2: Starting FastCGI backend on port 9000..."
# Create a simple FastCGI wrapper
cat > /tmp/fcgi_wrapper.py << 'EOF'
#!/usr/bin/env python3
from flup.server.fcgi import WSGIServer
import subprocess
import os

def application(environ, start_response):
    script = environ.get('SCRIPT_FILENAME', '')
    if not os.path.isfile(script):
        start_response('404 Not Found', [('Content-Type', 'text/plain')])
        return [b'Script not found']
    
    # Set up environment
    env = os.environ.copy()
    for key, value in environ.items():
        if isinstance(value, str):
            env[key] = value
    
    # Execute the script
    try:
        proc = subprocess.Popen(
            ['python3', script],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env
        )
        
        # Get input body if present
        content_length = int(environ.get('CONTENT_LENGTH', 0))
        body = environ['wsgi.input'].read(content_length) if content_length > 0 else b''
        
        stdout, stderr = proc.communicate(input=body)
        
        # Parse CGI output
        output = stdout.decode('utf-8', errors='replace')
        if '\r\n\r\n' in output:
            headers_part, body_part = output.split('\r\n\r\n', 1)
        elif '\n\n' in output:
            headers_part, body_part = output.split('\n\n', 1)
        else:
            headers_part = ''
            body_part = output
        
        # Parse headers
        status = '200 OK'
        response_headers = []
        for line in headers_part.split('\n'):
            line = line.strip()
            if not line:
                continue
            if line.startswith('Status:'):
                status = line.split(':', 1)[1].strip()
            elif ':' in line:
                key, value = line.split(':', 1)
                response_headers.append((key.strip(), value.strip()))
        
        start_response(status, response_headers)
        return [body_part.encode('utf-8')]
        
    except Exception as e:
        start_response('500 Internal Server Error', [('Content-Type', 'text/plain')])
        return [f'Error: {str(e)}'.encode()]

if __name__ == '__main__':
    WSGIServer(application, bindAddress=('127.0.0.1', 9000)).run()
EOF

chmod +x /tmp/fcgi_wrapper.py
python3 /tmp/fcgi_wrapper.py &
FCGI_PID=$!

echo -e "${GREEN}✓ FastCGI backend started (PID: $FCGI_PID)${NC}"
sleep 2

echo ""
echo "Step 3: Starting webserv..."
cd "$PROJECT_ROOT"
./webserv config/fastcgi_test.conf &
SERVER_PID=$!

echo -e "${GREEN}✓ webserv started (PID: $SERVER_PID)${NC}"
sleep 2

echo ""
echo "Step 4: Compiling and running tests..."
c++ -std=c++98 -Wall -Wextra -Werror tests/fastcgi_tests.cpp -o tests/fastcgi_tests || {
    echo -e "${RED}Failed to compile tests${NC}"
    kill $SERVER_PID $FCGI_PID 2>/dev/null
    exit 1
}

./tests/fastcgi_tests
TEST_RESULT=$?

echo ""
echo "Step 5: Cleanup..."
kill $SERVER_PID $FCGI_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
wait $FCGI_PID 2>/dev/null || true
rm -f /tmp/fcgi_wrapper.py

echo -e "${GREEN}✓ Cleanup complete${NC}"
echo ""

if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}All tests passed!${NC}"
    echo -e "${GREEN}=========================================${NC}"
else
    echo -e "${RED}=========================================${NC}"
    echo -e "${RED}Some tests failed${NC}"
    echo -e "${RED}=========================================${NC}"
fi

exit $TEST_RESULT
