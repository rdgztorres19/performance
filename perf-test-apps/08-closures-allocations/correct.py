#!/usr/bin/env python3
"""
CORRECT: No closure - pass threshold as arg or use module-level.
Resume: "Avoid Closures in Hot Paths"
"""
import os
import signal
import time
N = 1_000_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False
THRESHOLD = 50

def filter_above(data, thresh):
    total = 0
    for x in data:
        if x > thresh:
            total += x
    return total

def main():
    global stop
    data = list(range(100))
    print("CORRECT: Direct comparison, no closure")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        total = 0
        for _ in range(N):
            total += filter_above(data, THRESHOLD)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
