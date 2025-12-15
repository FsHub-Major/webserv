#!/usr/bin/env python3
"""Stress test script for webserv using asyncio.

Opens many concurrent TCP connections and performs a single GET request per connection.
Designed to scale to tens of thousands of clients but requires adequate OS limits (ulimit -n).

Usage examples:
  # quick smoke test (100 clients)
  ./scripts/stress_test.py --host 127.0.0.1 --port 8080 --clients 100

  # full-scale test (15k) with a 30s ramp-up
  sudo sysctl -w net.core.somaxconn=10000    # (optional, may require sudo)
  ulimit -n 65536
  ./scripts/stress_test.py --host 127.0.0.1 --port 8080 --clients 15000 --ramp 30

Note: The script will not change system limits. It will warn if current RLIMIT_NOFILE is likely too low.
"""

from __future__ import annotations
import argparse
import asyncio
import time
import sys
import resource
from typing import Tuple

DEFAULT_TIMEOUT = 10.0


async def single_request(host: str, port: int, path: str, timeout: float) -> Tuple[bool, float, int, str]:
    """Perform a single HTTP/1.1 GET request to host:port and return (success, latency, bytes_read, status_line)."""
    start = time.monotonic()
    try:
        reader, writer = await asyncio.open_connection(host, port)
    except Exception as e:
        return (False, 0.0, 0, f"connect_err:{e}")

    req = f"GET {path} HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\n\r\n"
    try:
        writer.write(req.encode('ascii'))
        await writer.drain()
    except Exception as e:
        try:
            writer.close()
        except Exception:
            pass
        return (False, 0.0, 0, f"send_err:{e}")

    bytes_read = 0
    status_line = ''
    try:
        # read header lines until blank line
        headers = b''
        # Use a timeout for the whole read
        async with asyncio.timeout(timeout):
            while True:
                chunk = await reader.read(1024)
                if not chunk:
                    break
                bytes_read += len(chunk)
                headers += chunk
                # detect end of headers
                if b'\r\n\r\n' in headers or b'\n\n' in headers:
                    break
            # try to extract status line
            try:
                s = headers.decode('utf-8', errors='replace')
                first_line = s.splitlines()[0] if s.splitlines() else ''
                status_line = first_line
            except Exception:
                status_line = ''
    except asyncio.TimeoutError:
        try:
            writer.close()
        except Exception:
            pass
        return (False, 0.0, bytes_read, 'timeout')
    except Exception as e:
        try:
            writer.close()
        except Exception:
            pass
        return (False, 0.0, bytes_read, f'read_err:{e}')

    try:
        writer.close()
    except Exception:
        pass

    latency = time.monotonic() - start
    return (True, latency, bytes_read, status_line)


async def run_load(host: str, port: int, clients: int, path: str, timeout: float, ramp: float, concurrency: int):
    tasks = []
    results = {
        'success': 0,
        'connect_err': 0,
        'timeout': 0,
        'other_err': 0,
        'bytes': 0,
        'latencies': []
    }

    start_time = time.monotonic()
    started = 0
    sem = asyncio.Semaphore(concurrency)

    async def worker(idx: int):
        async with sem:
            ok, lat, b, status = await single_request(host, port, path, timeout)
            if not ok:
                if status.startswith('connect_err'):
                    results['connect_err'] += 1
                elif status == 'timeout':
                    results['timeout'] += 1
                else:
                    results['other_err'] += 1
            else:
                results['success'] += 1
                results['latencies'].append(lat)
                results['bytes'] += b

    # Ramp: spread client task creation over 'ramp' seconds
    if ramp <= 0:
        # fire all at once
        coro = [asyncio.create_task(worker(i)) for i in range(clients)]
        started = clients
        tasks.extend(coro)
    else:
        interval = float(ramp) / max(1, clients)
        for i in range(clients):
            tasks.append(asyncio.create_task(worker(i)))
            started += 1
            # sleep a bit to ramp up
            await asyncio.sleep(interval)

    # Wait for completion
    await asyncio.gather(*tasks)
    elapsed = time.monotonic() - start_time

    # summary
    total = clients
    succ = results['success']
    conn_err = results['connect_err']
    tout = results['timeout']
    other = results['other_err']
    avg_lat = (sum(results['latencies']) / len(results['latencies'])) if results['latencies'] else 0.0

    print('\n==== Stress Test Summary ====')
    print(f'Clients requested: {clients}')
    print(f'Elapsed time: {elapsed:.2f}s')
    print(f'Successful responses: {succ}')
    print(f'Connection errors: {conn_err}')
    print(f'Timeouts: {tout}')
    print(f'Other errors: {other}')
    print(f'Avg latency (s): {avg_lat:.4f}')
    print('Total bytes received: {}'.format(results['bytes']))
    print('=============================\n')


def check_rlimit(clients: int):
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    est_needed = clients * 2 + 100
    if soft < est_needed:
        print(f"WARNING: current RLIMIT_NOFILE={soft}, estimated needed={est_needed}")
        print("You may need to increase the file descriptor limit (e.g. 'ulimit -n 65536') before running very large tests.")


def parse_args():
    p = argparse.ArgumentParser(description='Async stress tester for webserv')
    p.add_argument('--host', default='127.0.0.1', help='Host to target')
    p.add_argument('--port', type=int, default=8080, help='Port to target')
    p.add_argument('--clients', type=int, default=1000, help='Number of concurrent clients')
    p.add_argument('--path', default='/', help='Request path')
    p.add_argument('--timeout', type=float, default=DEFAULT_TIMEOUT, help='Per-request timeout (s)')
    p.add_argument('--ramp', type=float, default=0.0, help='Ramp-up time in seconds (spread client starts)')
    p.add_argument('--concurrency', type=int, default=1000, help='Internal concurrency semaphore limit per process')
    return p.parse_args()


def main():
    args = parse_args()

    print('Stress test parameters:')
    print(f' - host: {args.host}\n - port: {args.port}\n - clients: {args.clients}\n - path: {args.path}\n - timeout: {args.timeout}s\n - ramp: {args.ramp}s\n - concurrency: {args.concurrency}')

    check_rlimit(args.clients)

    try:
        asyncio.run(run_load(args.host, args.port, args.clients, args.path, args.timeout, args.ramp, args.concurrency))
    except KeyboardInterrupt:
        print('Interrupted by user')
        sys.exit(1)


if __name__ == '__main__':
    main()
