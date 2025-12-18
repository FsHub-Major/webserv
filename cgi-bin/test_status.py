#!/usr/bin/env python3
"""Test script for custom status codes"""
import os

query = os.environ.get('QUERY_STRING', '')
status_code = "200"
reason = "OK"

# Parse query string for status code (e.g., ?code=404)
if 'code=' in query:
    status_code = query.split('code=')[1].split('&')[0]
    if status_code == "404":
        reason = "Not Found"
    elif status_code == "500":
        reason = "Internal Server Error"
    elif status_code == "301":
        reason = "Moved Permanently"

print(f"Status: {status_code} {reason}")
print("Content-Type: text/plain")
print()
print(f"=== Status Code Test ===")
print(f"Custom status: {status_code} {reason}")
