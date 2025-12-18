#!/usr/bin/env python3
"""Test script for custom headers"""
import os

print("Content-Type: application/json")
print("X-Custom-Header: test-value")
print("Cache-Control: no-cache, no-store")
print("X-Powered-By: FastCGI/Python")
print()
print('{"status":"success","test":"custom_headers","method":"' + os.environ.get('REQUEST_METHOD', '') + '"}')
