#!/usr/bin/env python3
"""
INCORRECT: Per-item processing - function call overhead, poor cache locality.
Resume: "Process Data in Batches"
Run: DURATION_SEC=60 python incorrect.py
"""
import os
import signal
import time
N = 100_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def process_item(x):
    return x * 2 + 1

def main():
    global stop
    print("INCORRECT: Per-item processing")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        result = 0
        for i in range(N):
            result += process_item(i)
        laps += 1
    print(f"Stopped: {laps} laps in {time.perf_counter()-start:.1f}s")

if __name__ == "__main__":
    main()
