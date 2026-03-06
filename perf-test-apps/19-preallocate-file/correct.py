#!/usr/bin/env python3
"""
CORRECT: Preallocate with truncate/seek.
Resume: "Preallocate File Space"
"""
import os
import tempfile
import time
CHUNKS = 1000
CHUNK = 1024 * 1024
TOTAL = CHUNKS * CHUNK

def main():
    fd, path = tempfile.mkstemp()
    print("CORRECT: Preallocate, then write")
    start = time.perf_counter()
    with os.fdopen(fd, 'wb') as f:
        f.seek(TOTAL - 1)
        f.write(b'\x00')
        f.seek(0)
        for _ in range(CHUNKS):
            f.write(b'x' * CHUNK)
    elapsed = time.perf_counter() - start
    os.unlink(path)
    print(f"Prealloc+write {CHUNKS}MB in {elapsed:.2f}s")

if __name__ == "__main__":
    main()
