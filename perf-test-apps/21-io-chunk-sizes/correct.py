#!/usr/bin/env python3
"""
CORRECT: 64KB buffer - fewer syscalls.
Resume: "Choose Correct I/O Chunk Sizes"
"""
import os
import tempfile
import time
SIZE_MB = 20
CHUNK = 64 * 1024

def main():
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(os.urandom(SIZE_MB * 1024 * 1024))
        path = f.name
    try:
        print("CORRECT: 64KB buffer")
        start = time.perf_counter()
        with open(path, 'rb') as f:
            while f.read(CHUNK):
                pass
        print(f"Read {SIZE_MB}MB in {time.perf_counter()-start:.2f}s")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
