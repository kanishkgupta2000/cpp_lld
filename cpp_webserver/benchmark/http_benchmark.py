#!/usr/bin/env python3
"""
HTTP benchmark for cpp_webserver.

The server handles one request per TCP connection (no keep-alive), so each
benchmark request opens a fresh connection — matching real client behavior.

Usage:
  1. Build and start the server (from cpp_webserver/):
       g++ -std=c++23 -o server server_linux.cpp http_tcpServer_linux.cpp rest_pipeline.cpp
       ./server

  2. Run the benchmark:
       python3 benchmark/http_benchmark.py
       python3 benchmark/http_benchmark.py --requests 5000 --concurrency 50
       python3 benchmark/http_benchmark.py --duration 30 --concurrency 20
"""

from __future__ import annotations

import argparse
import socket
import statistics
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from typing import List, Optional


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080
DEFAULT_PATH = "/"
DEFAULT_METHOD = "GET"
REQUEST_TEMPLATE = (
    "{method} {path} HTTP/1.1\r\n"
    "Host: {host}:{port}\r\n"
    "Connection: close\r\n"
    "\r\n"
)


@dataclass
class RequestResult:
    ok: bool
    latency_ms: float
    status_code: Optional[int] = None
    bytes_received: int = 0
    error: Optional[str] = None


@dataclass
class BenchmarkStats:
    total: int = 0
    success: int = 0
    failed: int = 0
    latencies_ms: List[float] = field(default_factory=list)
    bytes_received: int = 0
    errors: dict[str, int] = field(default_factory=dict)

    def record(self, result: RequestResult) -> None:
        self.total += 1
        self.latencies_ms.append(result.latency_ms)
        self.bytes_received += result.bytes_received
        if result.ok:
            self.success += 1
        else:
            self.failed += 1
            key = result.error or f"HTTP {result.status_code}"
            self.errors[key] = self.errors.get(key, 0) + 1


def parse_status_code(response: bytes) -> Optional[int]:
    first_line = response.split(b"\r\n", 1)[0]
    parts = first_line.split()
    if len(parts) >= 2 and parts[0].startswith(b"HTTP/"):
        try:
            return int(parts[1])
        except ValueError:
            return None
    return None


def send_http_request(
    host: str,
    port: int,
    method: str,
    path: str,
    timeout_s: float,
) -> RequestResult:
    payload = REQUEST_TEMPLATE.format(
        method=method, path=path, host=host, port=port
    ).encode("ascii")
    response = b""
    start = time.perf_counter()

    try:
        with socket.create_connection((host, port), timeout=timeout_s) as sock:
            sock.sendall(payload)
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
    except OSError as exc:
        elapsed_ms = (time.perf_counter() - start) * 1000.0
        return RequestResult(ok=False, latency_ms=elapsed_ms, error=str(exc))

    elapsed_ms = (time.perf_counter() - start) * 1000.0
    status = parse_status_code(response)
    ok = status is not None and 200 <= status < 300

    return RequestResult(
        ok=ok,
        latency_ms=elapsed_ms,
        status_code=status,
        bytes_received=len(response),
        error=None if ok else f"HTTP {status}",
    )


def percentile(values: List[float], pct: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    rank = (pct / 100.0) * (len(ordered) - 1)
    lower = int(rank)
    upper = min(lower + 1, len(ordered) - 1)
    weight = rank - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def run_fixed_requests(
    host: str,
    port: int,
    method: str,
    path: str,
    total_requests: int,
    concurrency: int,
    timeout_s: float,
) -> tuple[BenchmarkStats, float]:
    stats = BenchmarkStats()
    lock = threading.Lock()
    started = time.perf_counter()

    def worker() -> None:
        result = send_http_request(host, port, method, path, timeout_s)
        with lock:
            stats.record(result)

    with ThreadPoolExecutor(max_workers=concurrency) as pool:
        futures = [pool.submit(worker) for _ in range(total_requests)]
        for future in as_completed(futures):
            future.result()

    elapsed_s = time.perf_counter() - started
    return stats, elapsed_s


def run_duration(
    host: str,
    port: int,
    method: str,
    path: str,
    duration_s: float,
    concurrency: int,
    timeout_s: float,
) -> tuple[BenchmarkStats, float]:
    stats = BenchmarkStats()
    lock = threading.Lock()
    stop_at = time.perf_counter() + duration_s
    stop_flag = threading.Event()

    def worker() -> None:
        while not stop_flag.is_set():
            if time.perf_counter() >= stop_at:
                break
            result = send_http_request(host, port, method, path, timeout_s)
            with lock:
                stats.record(result)

    started = time.perf_counter()
    with ThreadPoolExecutor(max_workers=concurrency) as pool:
        futures = [pool.submit(worker) for _ in range(concurrency)]
        time.sleep(duration_s)
        stop_flag.set()
        for future in as_completed(futures):
            future.result()

    elapsed_s = time.perf_counter() - started
    return stats, elapsed_s


def warmup(
    host: str,
    port: int,
    method: str,
    path: str,
    count: int,
    timeout_s: float,
) -> None:
    for _ in range(count):
        send_http_request(host, port, method, path, timeout_s)


def print_report(stats: BenchmarkStats, elapsed_s: float) -> None:
    latencies = stats.latencies_ms
    rps = stats.total / elapsed_s if elapsed_s > 0 else 0.0
    throughput_mib_s = (stats.bytes_received / elapsed_s / (1024 * 1024)) if elapsed_s > 0 else 0.0

    print("\n=== HTTP Server Benchmark Results ===")
    print(f"Elapsed time     : {elapsed_s:.3f} s")
    print(f"Total requests   : {stats.total}")
    print(f"Successful       : {stats.success}")
    print(f"Failed           : {stats.failed}")
    print(f"Requests/sec     : {rps:.2f}")
    print(f"Throughput       : {throughput_mib_s:.2f} MiB/s received")
    print(f"Bytes received   : {stats.bytes_received:,}")

    if latencies:
        print("\nLatency (ms)")
        print(f"  min            : {min(latencies):.2f}")
        print(f"  mean           : {statistics.mean(latencies):.2f}")
        print(f"  median         : {statistics.median(latencies):.2f}")
        if len(latencies) > 1:
            print(f"  stdev          : {statistics.stdev(latencies):.2f}")
        print(f"  p95            : {percentile(latencies, 95):.2f}")
        print(f"  p99            : {percentile(latencies, 99):.2f}")
        print(f"  max            : {max(latencies):.2f}")

    if stats.errors:
        print("\nErrors")
        for message, count in sorted(stats.errors.items(), key=lambda item: -item[1]):
            print(f"  {message}: {count}")

    print("=====================================\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark the cpp_webserver HTTP TCP server."
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help=f"Server host (default: {DEFAULT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Server port (default: {DEFAULT_PORT})")
    parser.add_argument("--path", default=DEFAULT_PATH, help=f"Request path (default: {DEFAULT_PATH})")
    parser.add_argument("--method", default=DEFAULT_METHOD, help=f"HTTP method (default: {DEFAULT_METHOD})")
    parser.add_argument("--concurrency", type=int, default=10, help="Concurrent workers (default: 10)")
    parser.add_argument("--requests", type=int, default=1000, help="Total requests for fixed-count mode (default: 1000)")
    parser.add_argument(
        "--duration",
        type=float,
        default=None,
        help="Run for this many seconds instead of a fixed request count",
    )
    parser.add_argument("--warmup", type=int, default=10, help="Warmup requests before measuring (default: 10)")
    parser.add_argument("--timeout", type=float, default=5.0, help="Per-request socket timeout in seconds (default: 5.0)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.concurrency < 1:
        print("error: --concurrency must be >= 1", file=sys.stderr)
        return 1
    if args.duration is None and args.requests < 1:
        print("error: --requests must be >= 1", file=sys.stderr)
        return 1
    if args.duration is not None and args.duration <= 0:
        print("error: --duration must be > 0", file=sys.stderr)
        return 1

    mode = f"duration={args.duration}s" if args.duration else f"requests={args.requests}"
    print("Starting benchmark")
    print(f"  target      : http://{args.host}:{args.port}{args.path}")
    print(f"  method      : {args.method}")
    print(f"  concurrency : {args.concurrency}")
    print(f"  mode        : {mode}")
    print(f"  warmup      : {args.warmup}")

    if args.warmup > 0:
        warmup(args.host, args.port, args.method, args.path, args.warmup, args.timeout)

    if args.duration is not None:
        stats, elapsed_s = run_duration(
            args.host,
            args.port,
            args.method,
            args.path,
            args.duration,
            args.concurrency,
            args.timeout,
        )
    else:
        stats, elapsed_s = run_fixed_requests(
            args.host,
            args.port,
            args.method,
            args.path,
            args.requests,
            args.concurrency,
            args.timeout,
        )

    print_report(stats, elapsed_s)
    return 0 if stats.failed == 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())
