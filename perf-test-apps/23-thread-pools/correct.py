#!/usr/bin/env python3
"""
CORRECT: ThreadPoolExecutor - reuse threads.
Resume: "Use Thread Pools"
"""
import os
import signal
import time
from concurrent.futures import ThreadPoolExecutor
TASKS = 500
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def task(_):
    return sum(range(1000))

def main():
    global stop
    print("CORRECT: ThreadPoolExecutor")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    with ThreadPoolExecutor(max_workers=8) as ex:
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            list(ex.map(task, range(TASKS)))
            laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
