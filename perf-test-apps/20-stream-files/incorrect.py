#!/usr/bin/env python3
"""
INCORRECT: Load entire file into memory.
Resume: "Stream Files Instead of Loading Entire Files"
One-shot - needs large file.
"""
import tempfile
import time
SIZE_MB = 50

def main():
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(b'x' * (SIZE_MB * 1024 * 1024))
        path = f.name
    try:
        print("INCORRECT: read() entire file")
        start = time.perf_counter()
        with open(path, 'rb') as f:
            data = f.read()
        elapsed = time.perf_counter() - start
        print(f"Loaded {len(data)//1024//1024}MB in {elapsed:.2f}s")
    finally:
        import os
        os.unlink(path)

if __name__ == "__main__":
    main()
