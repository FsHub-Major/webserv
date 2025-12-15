#!/usr/bin/env python3
"""Return a small JSON frame for animation.
Increments a counter stored in /tmp/webserv_anim_counter to produce changing frames.
"""
import os
import json
import time

COUNTER_FILE = '/tmp/webserv_anim_counter'

# Read/increment counter
cnt = 0
try:
    with open(COUNTER_FILE, 'r') as f:
        s = f.read().strip()
        if s.isdigit():
            cnt = int(s)
except Exception:
    cnt = 0
cnt += 1
try:
    with open(COUNTER_FILE, 'w') as f:
        f.write(str(cnt))
except Exception:
    pass

# derive frame properties
width = 600
height = 200
x = (cnt * 7) % width
y = height // 2
colors = ['#e11d48', '#0ea5e9', '#10b981', '#f59e0b', '#8b5cf6']
color = colors[cnt % len(colors)]

frame = {
    'frame': cnt,
    'x': x,
    'y': y,
    'color': color,
    'ts': int(time.time())
}

print('Content-Type: application/json')
print('Cache-Control: no-cache, no-store, must-revalidate')
print()
print(json.dumps(frame))
