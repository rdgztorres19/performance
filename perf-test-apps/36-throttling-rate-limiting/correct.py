#!/usr/bin/env python3
"""
CORRECT: Rate limit - max N per second.
Resume: "Use Throttling and Rate Limiting"
"""
import time
N = 100
RATE = 10  # per second
min_interval = 1.0 / RATE

def main():
    print("CORRECT: Rate limit 10/sec")
    start = time.perf_counter()
    last = start
    for i in range(N):
        now = time.perf_counter()
        elapsed = now - last
        if elapsed < min_interval:
            time.sleep(min_interval - elapsed)
        last = time.perf_counter()
    print(f"Processed {N} in {time.perf_counter()-start:.2f}s (throttled)")

if __name__ == "__main__":
    main()
