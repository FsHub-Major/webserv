#!/usr/bin/env python3
"""Test script for POST requests"""
import os
import sys

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_data = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/plain")
print()
print("=== FastCGI POST Test ===")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'NOT_SET')}")
print(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'NOT_SET')}")
print(f"CONTENT_LENGTH: {content_length}")
print(f"POST data received: {post_data}")
print("POST request successful!")
