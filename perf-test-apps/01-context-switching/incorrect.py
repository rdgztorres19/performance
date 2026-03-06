#!/usr/bin/env python3
"""
INCORRECT: Excessive context switching
Resume: "Reduce Context Switching" - More threads than CPU cores causes
constant rotation, cache eviction, and scheduler overhead.

Run: DURATION_SEC=60 python incorrect.py
Measure: vmstat 1, pidstat -w
"""

import os
import signal
import threading
import multiprocessing
import time

NUM_THREADS = 500  # Way more than typical CPU cores
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
    print(f"INCORRECT: {NUM_THREADS} threads (>> {num_cores} cores) -> context switch storm")
    print(f"Running {DURATION_SEC}s. Measure: vmstat 1, pidstat -w")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))

    threads = [threading.Thread(target=cpu_busy_work) for _ in range(NUM_THREADS)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join(timeout=2)
    print("Stopped")


if __name__ == "__main__":
    main()
