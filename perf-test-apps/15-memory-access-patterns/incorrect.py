#!/usr/bin/env python3
"""
INCORRECT: Random access - prefetcher can't help.
Resume: "Optimize Memory Access Patterns"
"""
import random
import signal
import time
N = 2_000_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    data = list(range(N))
    indices = list(range(N))
    random.shuffle(indices)
    print("INCORRECT: Random access")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    total = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        for i in indices:
            total += data[i]
        laps += 1
    print(f"Stopped: {laps} laps, total={total}")

if __name__ == "__main__":
    main()
