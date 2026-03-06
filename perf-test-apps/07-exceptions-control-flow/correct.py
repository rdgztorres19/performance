#!/usr/bin/env python3
"""
CORRECT: Check first, no exception for expected case.
Resume: "Avoid Exceptions for Control Flow"
"""
import os
import signal
import time
N = 100_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False
DATA = ["42", "invalid", "17", "bad", "99"] * (N // 5)

def parse_good(s):
    if s.isdigit() or (s.startswith('-') and s[1:].isdigit()):
        return int(s)
    return None

def main():
    global stop
    print("CORRECT: isdigit() check -> no exception")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        total = 0
        for s in DATA:
            v = parse_good(s)
            if v is not None:
                total += v
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
