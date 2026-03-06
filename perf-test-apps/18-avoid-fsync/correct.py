#!/usr/bin/env python3
"""
CORRECT: Batch writes, fsync once.
Resume: "Avoid Frequent fsync"
"""
import os
import tempfile
import time
N = 100

def main():
    fd, path = tempfile.mkstemp()
    print("CORRECT: Batch, fsync once")
    start = time.perf_counter()
    with os.fdopen(fd, 'wb') as f:
        for i in range(N):
            f.write(b'x' * 100)
        os.fsync(f.fileno())
    elapsed = time.perf_counter() - start
    os.unlink(path)
    print(f"{N} writes, 1 fsync in {elapsed:.2f}s")

if __name__ == "__main__":
    main()
