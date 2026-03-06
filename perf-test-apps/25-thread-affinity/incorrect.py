#!/usr/bin/env python3
"""
INCORRECT: No affinity - OS may migrate threads between cores.
Resume: "Thread Affinity"
Note: Requires Linux, runs without affinity (default behavior).
"""
import os
import signal
import threading
import multiprocessing
import time

NUM_THREADS = 4
ITERATIONS = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def worker():
    total = 0
    while not stop:
        for i in range(ITERATIONS):
            total += i
    return total

def main():
    global stop
    print("INCORRECT: No thread affinity (OS may migrate)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    threads = [threading.Thread(target=worker) for _ in range(NUM_THREADS)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join(timeout=2)
    print("Stopped")

if __name__ == "__main__":
    main()
