#!/usr/bin/env python3
"""
INCORRECT: Busy-wait (spin loop) - burns 100% CPU, never yields.
Resume: "Avoid Busy-Wait Loops"
Run: DURATION_SEC=60 python incorrect.py
"""
import os
import signal
import threading
import time

NUM_WAITERS = 4
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def busy_wait_until(deadline):
    while time.perf_counter() < deadline and not stop:
        pass  # Busy-wait: 100% CPU

def main():
    global stop
    print("INCORRECT: Busy-wait. Run: top -p $(pgrep -f busy)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    deadline = time.perf_counter() + DURATION_SEC
    threads = [threading.Thread(target=busy_wait_until, args=(deadline,)) for _ in range(NUM_WAITERS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    print("Stopped")

if __name__ == "__main__":
    main()
