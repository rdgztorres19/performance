#!/usr/bin/env python3
"""
INCORRECT: Grow file incrementally - fragmentation.
Resume: "Preallocate File Space"
"""
import os
import tempfile
import time
CHUNKS = 1000
CHUNK = 1024 * 1024  # 1MB

def main():
    fd, path = tempfile.mkstemp()
    print("INCORRECT: Append 1MB at a time, no prealloc")
    start = time.perf_counter()
    with os.fdopen(fd, 'wb') as f:
        for _ in range(CHUNKS):
            f.write(b'x' * CHUNK)
    elapsed = time.perf_counter() - start
    os.unlink(path)
    print(f"Wrote {CHUNKS}MB in {elapsed:.2f}s")

if __name__ == "__main__":
    main()
