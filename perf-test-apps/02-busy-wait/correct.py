#!/usr/bin/env python3
"""
CORRECT: Use threading.Event or time.sleep - yields to scheduler.
Resume: "Avoid Busy-Wait Loops" - Use proper sync primitives.
Run: DURATION_SEC=60 python correct.py
"""
import os
import signal
import threading
import time

NUM_WAITERS = 4
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop_event = threading.Event()

def wait_with_event(deadline):
    while time.perf_counter() < deadline and not stop_event.is_set():
        stop_event.wait(timeout=0.01)  # Yield to scheduler

def main():
    print("CORRECT: Event.wait() - yields to scheduler")
    signal.signal(signal.SIGINT, lambda s, f: stop_event.set())
    signal.signal(signal.SIGTERM, lambda s, f: stop_event.set())
    deadline = time.perf_counter() + DURATION_SEC
    threads = [threading.Thread(target=wait_with_event, args=(deadline,)) for _ in range(NUM_WAITERS)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop_event.set()
    for t in threads:
        t.join()
    print("Stopped")

if __name__ == "__main__":
    main()
