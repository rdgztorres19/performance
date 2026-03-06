#!/usr/bin/env python3
"""
CORRECT: Skip compression when I/O is fast / CPU-bound.
Resume: "Balance Compression vs I/O Cost"
"""
import time
N = 1000
DATA = b"x" * 10000

def main():
    print("CORRECT: No compression when CPU-bound")
    start = time.perf_counter()
    for _ in range(N):
        _ = DATA
    print(f"Processed {N} chunks in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
