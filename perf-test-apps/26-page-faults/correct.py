#!/usr/bin/env python3
"""
CORRECT: Pre-touch each page to trigger allocation upfront.
Resume: "Avoid Page Faults" - Pre-load pages into physical RAM.
"""
import time
SIZE_MB = 100
PAGE = 4096

def main():
    print("CORRECT: Pre-touch pages (warm)")
    arr = bytearray(SIZE_MB * 1024 * 1024)
    start = time.perf_counter()
    for i in range(0, len(arr), PAGE):
        arr[i] = 0
    elapsed = time.perf_counter() - start
    print(f"Pre-touched {SIZE_MB}MB in {elapsed:.3f}s")
    start2 = time.perf_counter()
    total = 0
    for i in range(0, len(arr), PAGE):
        total += arr[i]
    print(f"Second pass (warm): {time.perf_counter()-start2:.3f}s")

if __name__ == "__main__":
    main()
