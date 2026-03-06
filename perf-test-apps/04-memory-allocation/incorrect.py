#!/usr/bin/env python3
"""
INCORRECT: Many allocations, no pooling - GC pressure.
Resume: "Use Memory Pooling"
"""
import os
import signal
import time
N = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    print("INCORRECT: Excessive allocations")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        items = []
        for i in range(N):
            items.append({"id": i, "value": str(i) * 10})
        laps += 1
    print(f"Stopped: {laps} laps (fix: pooling)")

if __name__ == "__main__":
    main()
