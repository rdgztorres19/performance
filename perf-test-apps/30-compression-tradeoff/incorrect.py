#!/usr/bin/env python3
"""
INCORRECT: Compress when CPU-bound - adds CPU cost, no I/O gain.
Resume: "Balance Compression vs I/O Cost"
Simulates: CPU-bound path with gzip (wrong choice).
"""
import gzip
import io
import time
N = 1000
DATA = b"x" * 10000

def main():
    print("INCORRECT: Compress on CPU-bound path")
    start = time.perf_counter()
    for _ in range(N):
        buf = io.BytesIO()
        with gzip.GzipFile(fileobj=buf, mode='wb') as gz:
            gz.write(DATA)
        buf.getvalue()
    print(f"Compressed {N} chunks in {time.perf_counter()-start:.2f}s (CPU cost)")

if __name__ == "__main__":
    main()
