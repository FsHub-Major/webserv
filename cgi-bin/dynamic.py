#!/usr/bin/env python3
"""Simple CGI script that returns a changing counter and timestamp on each request.
It stores a small counter in /tmp/webserv_dynamic_counter to demonstrate changing output.
"""
import os
import time

COUNTER_FILE = '/tmp/webserv_dynamic_counter'

# Ensure file exists
try:
    with open(COUNTER_FILE, 'a+') as f:
        pass
except Exception:
    pass

# Read, increment, write
cnt = 0
try:
    with open(COUNTER_FILE, 'r') as f:
        data = f.read().strip()
        if data.isdigit():
            cnt = int(data)
except Exception:
    cnt = 0

cnt += 1
try:
    with open(COUNTER_FILE, 'w') as f:
        f.write(str(cnt))
except Exception:
    pass

# Output a simple HTML page
print('Content-Type: text/html')
print()
print('<!doctype html>')
print('<html><head><meta charset="utf-8"><title>Dynamic CGI</title></head><body>')
print('<h1>Dynamic CGI Test</h1>')
print('<p>Request number: {}</p>'.format(cnt))
print('<p>Timestamp: {}</p>'.format(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())))
print('<p>Process id: {}</p>'.format(os.getpid()))
print('<p>Environment REMOTE_ADDR: {}</p>'.format(os.environ.get('REMOTE_ADDR', 'unknown')))
print('</body></html>')
