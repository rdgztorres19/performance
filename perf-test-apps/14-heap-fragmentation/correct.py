#!/usr/bin/env python3
"""
CORRECT: Group by size - less fragmentation.
Resume: "Avoid Heap Fragmentation"
"""
import time
SMALL = 100
LARGE = 50_000
N = 10_000

def main():
    print("CORRECT: Group small, then large")
    start = time.perf_counter()
    smalls = [bytearray(SMALL) for _ in range(N)]
    larges = [bytearray(LARGE) for _ in range(N)]
    elapsed = time.perf_counter() - start
    print(f"Allocated {N*2} objects in {elapsed:.2f}s")

if __name__ == "__main__":
    main()
