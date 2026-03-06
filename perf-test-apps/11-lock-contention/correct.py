#!/usr/bin/env python3
"""
CORRECT: Lock-free with local accumulation, rare lock.
Resume: "Prefer Lock-Free" / reduce contention
"""
import os
import signal
import threading
import time
NUM_THREADS = 32
ITERATIONS = 50_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
lock = threading.Lock()
counter = 0
stop = False

def increment():
    global counter
    local = 0
    while not stop:
        for _ in range(ITERATIONS):
            local += 1
        with lock:
            counter += local
        local = 0

def main():
    global stop
    print("CORRECT: Local accum, lock once per batch")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    threads = [threading.Thread(target=increment) for _ in range(NUM_THREADS)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join(timeout=2)
    print(f"Stopped. counter={counter}")

if __name__ == "__main__":
    main()
