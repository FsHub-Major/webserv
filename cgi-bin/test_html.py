#!/usr/bin/env python3
"""Test script for HTML output"""
import os

print("Content-Type: text/html")
print()
print("""<!DOCTYPE html>
<html>
<head>
    <title>FastCGI HTML Test</title>
</head>
<body>
    <h1>FastCGI HTML Output Test</h1>
    <p><strong>Method:</strong> {}</p>
    <p><strong>Query String:</strong> {}</p>
    <p><strong>Server:</strong> {}:{}</p>
    <p>If you see this page, FastCGI HTML rendering works!</p>
</body>
</html>""".format(
    os.environ.get('REQUEST_METHOD', 'UNKNOWN'),
    os.environ.get('QUERY_STRING', 'none'),
    os.environ.get('SERVER_NAME', 'unknown'),
    os.environ.get('SERVER_PORT', 'unknown')
))
