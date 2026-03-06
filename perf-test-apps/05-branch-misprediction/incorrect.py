#!/usr/bin/env python3
"""
INCORRECT: Unpredictable branches - ~50% branch miss rate.
Resume: "Branch Prediction"
Run: perf stat -e branches,branch-misses python incorrect.py
"""
import os
import random
import signal
import time
N = 5_000_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def sum_unpredictable(data):
    total = 0
    for x in data:
        if x % 2 == 0:
            total += x
        else:
            total -= x
    return total

def main():
    global stop
    data = [random.randint(0, 99) for _ in range(N)]
    print("INCORRECT: Unpredictable branches")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        sum_unpredictable(data)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
