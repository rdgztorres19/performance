#!/usr/bin/env python3
"""
CORRECT: Pin threads to cores (Linux only, needs sched).
Resume: "Thread Affinity"
"""
import os
import signal
import threading
import multiprocessing
import time

NUM_THREADS = min(4, multiprocessing.cpu_count())
ITERATIONS = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def worker(core_id):
    try:
        os.sched_setaffinity(0, {core_id % os.cpu_count()})
    except (AttributeError, OSError):
        pass  # Non-Linux or no permission
    total = 0
    while not stop:
        for i in range(ITERATIONS):
            total += i
    return total

def main():
    global stop
    if hasattr(os, 'sched_setaffinity'):
        print("CORRECT: Thread affinity (pinned to cores)")
    else:
        print("CORRECT: (sched_setaffinity not available on this OS)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    threads = [threading.Thread(target=worker, args=(i,)) for i in range(NUM_THREADS)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join(timeout=2)
    print("Stopped")

if __name__ == "__main__":
    main()
