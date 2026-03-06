#!/usr/bin/env python3
"""
INCORRECT: Read entire file into memory.
Resume: "Use Memory-Mapped I/O"
"""
import os
import tempfile
import time
SIZE_MB = 100

def main():
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(os.urandom(SIZE_MB * 1024 * 1024))
        path = f.name
    try:
        print("INCORRECT: read() entire file")
        start = time.perf_counter()
        with open(path, 'rb') as f:
            data = f.read()
        total = sum(data[i] for i in range(0, len(data), 4096))
        print(f"Read {SIZE_MB}MB in {time.perf_counter()-start:.2f}s, sum={total}")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
