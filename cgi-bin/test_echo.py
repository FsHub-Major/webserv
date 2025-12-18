#!/usr/bin/env python3
"""Echo script - returns what it receives"""
import os
import sys

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/plain")
print()
print("=== Echo Test ===")
print(f"You sent: {body}")
print(f"Length: {len(body)} bytes")
