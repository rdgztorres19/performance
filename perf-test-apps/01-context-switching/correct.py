#!/usr/bin/env python3
"""
CORRECT: Limit threads to CPU cores
Resume: "Reduce Context Switching" - Match thread count to cores for CPU-bound work.

Run: DURATION_SEC=60 python correct.py
Measure: vmstat 1, pidstat -w -> fewer context switches
"""

import os
import signal
import threading
import multiprocessing
import time

ITERATIONS = 100_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False


def cpu_busy_work():
    total = 0
    while not stop:
        for i in range(ITERATIONS):
            total += i * (i + 1)
    return total


def main():
    global stop
    num_cores = multiprocessing.cpu_count()
    num_threads = num_cores  # Match threads to cores
    print(f"CORRECT: {num_threads} threads (= {num_cores} cores)")
    print(f"Running {DURATION_SEC}s. Measure: vmstat 1, pidstat -w")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))

    threads = [threading.Thread(target=cpu_busy_work) for _ in range(num_threads)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join(timeout=2)
    print("Stopped")


if __name__ == "__main__":
    main()
