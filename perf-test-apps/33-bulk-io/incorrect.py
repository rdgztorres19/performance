#!/usr/bin/env python3
"""
INCORRECT: Byte-by-byte read.
Resume: "Use Bulk Read and Write Operations"
"""
import os
import tempfile
import time
SIZE_KB = 1000

def main():
    fd, path = tempfile.mkstemp()
    os.ftruncate(fd, SIZE_KB * 1024)
    print("INCORRECT: read(1) per byte")
    start = time.perf_counter()
    with os.fdopen(fd, 'rb') as f:
        while f.read(1):
            pass
    print(f"Read {SIZE_KB}KB in {time.perf_counter()-start:.2f}s")
    os.unlink(path)

if __name__ == "__main__":
    main()
