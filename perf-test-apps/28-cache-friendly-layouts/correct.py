#!/usr/bin/env python3
"""
CORRECT: Struct of Arrays - iterating ids loads only ids.
Resume: "Use Cache-Friendly Memory Layouts"
"""
import os
import signal
import time
N = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def main():
    global stop
    ids = list(range(N))
    print("CORRECT: SoA - only load ids")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        total = sum(ids)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
