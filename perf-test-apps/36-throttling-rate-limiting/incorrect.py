#!/usr/bin/env python3
"""
INCORRECT: No rate limit - burst all at once.
Resume: "Use Throttling and Rate Limiting"
"""
import time
N = 100

def process(i):
    pass

def main():
    print("INCORRECT: No throttling - burst 100")
    start = time.perf_counter()
    for i in range(N):
        process(i)
    print(f"Processed {N} in {time.perf_counter()-start:.3f}s (burst)")

if __name__ == "__main__":
    main()
