#!/usr/bin/env python3
"""
INCORRECT: Create new thread per task - overhead.
Resume: "Use Thread Pools"
"""
import os
import signal
import threading
import time
TASKS = 500
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def task():
    return sum(range(1000))

def main():
    global stop
    print("INCORRECT: new Thread per task")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        threads = [threading.Thread(target=task) for _ in range(TASKS)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
