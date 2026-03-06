#!/usr/bin/env python3
"""
CORRECT: Stream in chunks - constant memory.
Resume: "Stream Files Instead of Loading Entire Files"
"""
import os
import tempfile
import time
SIZE_MB = 50
CHUNK = 64 * 1024

def main():
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(b'x' * (SIZE_MB * 1024 * 1024))
        path = f.name
    try:
        print("CORRECT: read in 64KB chunks")
        start = time.perf_counter()
        total = 0
        with open(path, 'rb') as f:
            while True:
                chunk = f.read(CHUNK)
                if not chunk:
                    break
                total += len(chunk)
        elapsed = time.perf_counter() - start
        print(f"Streamed {total//1024//1024}MB in {elapsed:.2f}s")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
