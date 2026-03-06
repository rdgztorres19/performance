#!/usr/bin/env python3
"""
CORRECT: Sort data first - predictable branches. Or use branchless.
Resume: "Branch Prediction"
"""
import os
import signal
import time
N = 5_000_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

# Sorted: evens first, then odds -> predictable
def sum_predictable(data):
    total = 0
    for x in data:
        if x % 2 == 0:
            total += x
        else:
            total -= x
    return total

def main():
    global stop
    data = list(range(100)) * (N // 100)
    data.sort(key=lambda x: x % 2)  # Evens first -> predictable
    print("CORRECT: Sorted data -> predictable branches")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        sum_predictable(data)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
