#!/usr/bin/env python3
"""
INCORRECT: Heavy lock contention - many threads, one lock.
Resume: "Minimize Lock Contention"
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
    while not stop:
        for _ in range(ITERATIONS):
            with lock:
                counter += 1

def main():
    global stop
    print("INCORRECT: 32 threads, 1 lock")
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
