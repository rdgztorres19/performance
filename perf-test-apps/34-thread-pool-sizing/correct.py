#!/usr/bin/env python3
"""
CORRECT: Worker threads = CPU cores for CPU-bound.
Resume: "Proper Thread Pool Sizing"
"""
import os
import signal
import time
from concurrent.futures import ThreadPoolExecutor
import multiprocessing
TASKS = 200
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def cpu_task(_):
    return sum(range(10000))

def main():
    global stop
    n = multiprocessing.cpu_count()
    print(f"CORRECT: {n} workers (= cores)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    with ThreadPoolExecutor(max_workers=n) as ex:
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            list(ex.map(cpu_task, range(TASKS)))
            laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
