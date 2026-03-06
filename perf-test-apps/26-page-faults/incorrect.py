#!/usr/bin/env python3
"""
INCORRECT: Touch large allocation without pre-loading - page faults on first access.
Resume: "Avoid Page Faults"
"""
import time
SIZE_MB = 100

def main():
    print("INCORRECT: Allocate, access cold pages (page faults)")
    start = time.perf_counter()
    arr = bytearray(SIZE_MB * 1024 * 1024)
    for i in range(0, len(arr), 4096):
        arr[i] = 1
    elapsed = time.perf_counter() - start
    print(f"Touched {SIZE_MB}MB in {elapsed:.3f}s (cold)")

if __name__ == "__main__":
    main()
