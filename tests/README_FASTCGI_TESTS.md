# FastCGI Testing Suite

Complete test suite for validating FastCGI implementation in webserv.

## Test Files Created

### 1. Test Scripts (`cgi-bin/`)
- **`test_get.py`** - Tests GET requests and query parameters
- **`test_post.py`** - Tests POST requests and body handling
- **`test_status.py`** - Tests custom HTTP status codes
- **`test_headers.py`** - Tests custom HTTP headers
- **`test_html.py`** - Tests HTML content generation
- **`test_echo.py`** - Tests request body echoing

### 2. Configuration
- **`config/fastcgi_test.conf`** - Server configuration for testing

### 3. Automated Tests
- **`tests/fastcgi_tests.cpp`** - C++ integration test suite
- **`tests/run_fastcgi_tests.sh`** - Automated test runner script

### 4. Documentation
- **`tests/FASTCGI_TESTING.md`** - Manual testing guide

## Quick Start

### Automated Testing (Recommended)

```bash
# Build the server
make

# Run all tests automatically
./tests/run_fastcgi_tests.sh
```

The script will:
1. Install flup if needed
2. Start FastCGI backend on port 9000
3. Start webserv on port 8080
4. Compile and run test suite
5. Clean up processes
6. Report results

### Manual Testing

#### Step 1: Start FastCGI Backend

```bash
pip3 install flup6  # or flup
python3 -c "
from flup.server.fcgi import WSGIServer
import subprocess, os

def app(env, start_response):
    script = env.get('SCRIPT_FILENAME', '')
    if not os.path.isfile(script):
        start_response('404 Not Found', [('Content-Type', 'text/plain')])
        return [b'Not found']
    
    proc = subprocess.Popen(['python3', script],
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           env={**os.environ, **{k:v for k,v in env.items() if isinstance(v,str)}})
    
    content_length = int(env.get('CONTENT_LENGTH', 0))
    body = env['wsgi.input'].read(content_length) if content_length > 0 else b''
    stdout, _ = proc.communicate(body)
    
    output = stdout.decode('utf-8', errors='replace')
    if '\r\n\r\n' in output:
        headers_part, body_part = output.split('\r\n\r\n', 1)
    elif '\n\n' in output:
        headers_part, body_part = output.split('\n\n', 1)
    else:
        headers_part, body_part = '', output
    
    status = '200 OK'
    headers = []
    for line in headers_part.split('\n'):
        line = line.strip()
        if line.startswith('Status:'):
            status = line.split(':', 1)[1].strip()
        elif ':' in line:
            k, v = line.split(':', 1)
            headers.append((k.strip(), v.strip()))
    
    start_response(status, headers)
    return [body_part.encode('utf-8')]

WSGIServer(app, bindAddress=('127.0.0.1', 9000)).run()
"
```

#### Step 2: Start webserv

```bash
./webserv config/fastcgi_test.conf
```

#### Step 3: Run Tests

```bash
# Using curl
curl http://localhost:8080/cgi-bin/test_get.py?name=test
curl -X POST -d 'data=value' http://localhost:8080/cgi-bin/test_post.py
curl -i http://localhost:8080/cgi-bin/test_status.py?code=404
curl -i http://localhost:8080/cgi-bin/test_headers.py
curl http://localhost:8080/cgi-bin/test_html.py
curl -X POST -d 'Echo me!' http://localhost:8080/cgi-bin/test_echo.py

# Or compile and run C++ tests
c++ -std=c++98 -Wall -Wextra tests/fastcgi_tests.cpp -o tests/fastcgi_tests
./tests/fastcgi_tests
```

## Test Coverage

### ✓ GET Requests
- Basic GET functionality
- Query string parsing
- CGI environment variables
- Response headers

### ✓ POST Requests
- Request body transmission
- Content-Type handling
- Content-Length validation
- Body data parsing

### ✓ Status Codes
- Default 200 OK
- Custom status codes (404, 500, etc.)
- Status line formatting

### ✓ Headers
- Content-Type handling
- Custom header propagation
- Standard CGI headers
- Response header parsing

### ✓ Content Types
- text/plain
- text/html
- application/json
- Custom MIME types

### ✓ Error Handling
- 404 for missing scripts
- 502 for backend failures
- 504 for timeouts
- Invalid fastcgi_pass configuration

## Expected Output

```
==================================
FastCGI Integration Tests
==================================
Prerequisites:
1. Server running on 127.0.0.1:8080
2. FastCGI backend (python-flup) running on port 9000
3. Test scripts in cgi-bin/ directory
==================================

Running tests...

✓ FastCGI GET: Returns 200 OK
✓ FastCGI GET: Has correct content type
✓ FastCGI GET: Contains expected output
✓ FastCGI GET: Query string passed

✓ FastCGI POST: Returns 200 OK
✓ FastCGI POST: Body received
✓ FastCGI POST: Content-Length passed

✓ FastCGI Status: Custom 404
✓ FastCGI Status: Custom 500

✓ FastCGI Headers: JSON content type
✓ FastCGI Headers: Custom header
✓ FastCGI Headers: JSON body

✓ FastCGI HTML: Returns 200 OK
✓ FastCGI HTML: HTML content type
✓ FastCGI HTML: Contains HTML tags

✓ FastCGI Echo: Returns 200 OK
✓ FastCGI Echo: Message echoed back

✓ FastCGI Error: 404 for missing script

==================================
Test Results:
  Passed: 17
  Failed: 0
  Total:  17
==================================
```

## Troubleshooting

### FastCGI backend not responding
```bash
# Check if port 9000 is in use
lsof -i :9000

# Kill existing process
kill $(lsof -t -i:9000)
```

### Connection refused
- Ensure FastCGI backend started before webserv
- Check `fastcgi_pass` directive in config
- Verify port 9000 is accessible

### Scripts not executing
```bash
# Ensure scripts are executable
chmod +x cgi-bin/test_*.py

# Verify Python path
which python3
```

### Tests fail to connect
- Verify webserv is running on port 8080
- Check firewall settings
- Ensure config file path is correct

## Additional Test Ideas

You can extend the test suite with:
- Large POST body handling
- Binary data transmission
- Multiple concurrent requests
- Timeout testing
- Error recovery
- PHP-FPM integration (if available)
- CGI script errors
- Environment variable validation
