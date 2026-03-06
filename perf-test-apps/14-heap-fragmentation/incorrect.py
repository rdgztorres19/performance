#!/usr/bin/env python3
"""
INCORRECT: Mix small and large allocations - fragmentation.
Resume: "Avoid Heap Fragmentation"
One-shot: creates fragmentation pattern.
"""
import time
SMALL = 100
LARGE = 50_000
N = 10_000

def main():
    print("INCORRECT: Interleaved small+large -> fragmentation")
    start = time.perf_counter()
    objs = []
    for i in range(N):
        objs.append(bytearray(SMALL))
        objs.append(bytearray(LARGE))
    elapsed = time.perf_counter() - start
    print(f"Allocated {N*2} objects in {elapsed:.2f}s")
    print("Fix: group by size, use pools")

if __name__ == "__main__":
    main()
