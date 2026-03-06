#!/usr/bin/env python3
"""
INCORRECT: Closures in hot path - capture creates heap allocations.
Resume: "Avoid Closures in Hot Paths"
"""
import os
import signal
import time
N = 1_000_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    threshold = 50
    data = list(range(100))
    print("INCORRECT: listcomp with closure (captures threshold)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        total = 0
        for _ in range(N):
            filtered = [x for x in data if x > threshold]
            total += sum(filtered)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
