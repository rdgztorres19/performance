#!/usr/bin/env python3
"""
CORRECT: Batch processing - single pass, better cache locality.
Resume: "Process Data in Batches"
"""
import os
import signal
import time
N = 100_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def process_batch(data):
    total = 0
    for x in data:
        total += x * 2 + 1
    return total

def main():
    global stop
    print("CORRECT: Batch processing")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    data = list(range(N))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        process_batch(data)
        laps += 1
    print(f"Stopped: {laps} laps in {time.perf_counter()-start:.1f}s")

if __name__ == "__main__":
    main()
