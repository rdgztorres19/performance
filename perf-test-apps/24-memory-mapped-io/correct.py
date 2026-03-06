#!/usr/bin/env python3
"""
CORRECT: mmap - OS loads pages on demand.
Resume: "Use Memory-Mapped I/O"
"""
import mmap
import os
import tempfile
import time
SIZE_MB = 100

def main():
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(os.urandom(SIZE_MB * 1024 * 1024))
        path = f.name
    try:
        print("CORRECT: mmap")
        start = time.perf_counter()
        with open(path, 'r+b') as f:
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
                total = sum(m[i] for i in range(0, len(m), 4096))
        print(f"mmap {SIZE_MB}MB in {time.perf_counter()-start:.2f}s, sum={total}")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
