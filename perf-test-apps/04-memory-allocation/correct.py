#!/usr/bin/env python3
"""
CORRECT: Reuse/pre-allocate - less GC pressure.
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
    print("CORRECT: Pre-allocated, reuse")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    # Pre-allocate and reuse
    items = [{"id": 0, "value": ""} for _ in range(N)]
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        for i in range(N):
            items[i]["id"] = i
            items[i]["value"] = str(i) * 10
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
