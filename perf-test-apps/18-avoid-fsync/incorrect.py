#!/usr/bin/env python3
"""
INCORRECT: fsync after every write - very slow.
Resume: "Avoid Frequent fsync"
One-shot or short run - fsync is expensive.
"""
import os
import tempfile
import time
N = 100

def main():
    fd, path = tempfile.mkstemp()
    print("INCORRECT: fsync per write")
    start = time.perf_counter()
    with os.fdopen(fd, 'wb') as f:
        for i in range(N):
            f.write(b'x' * 100)
            os.fsync(f.fileno())
    elapsed = time.perf_counter() - start
    os.unlink(path)
    print(f"{N} writes+fsync in {elapsed:.2f}s")

if __name__ == "__main__":
    main()
