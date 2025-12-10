#!/usr/bin/env python3
"""Test script for GET requests"""
import os
import sys

print("Content-Type: text/plain")
print()
print("=== FastCGI GET Test ===")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'NOT_SET')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', '')}")
print(f"SCRIPT_FILENAME: {os.environ.get('SCRIPT_FILENAME', 'NOT_SET')}")
print(f"SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'NOT_SET')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'NOT_SET')}")
print(f"SERVER_NAME: {os.environ.get('SERVER_NAME', 'NOT_SET')}")
print(f"SERVER_PORT: {os.environ.get('SERVER_PORT', 'NOT_SET')}")
print(f"SERVER_PROTOCOL: {os.environ.get('SERVER_PROTOCOL', 'NOT_SET')}")
print(f"GATEWAY_INTERFACE: {os.environ.get('GATEWAY_INTERFACE', 'NOT_SET')}")
print("GET request successful!")
