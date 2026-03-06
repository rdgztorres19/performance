#!/usr/bin/env python3
"""
INCORRECT: Allocate buffer each time - no pooling.
Resume: "Use Memory Pooling"
"""
import os
import signal
import time
N = 100_000
BUF_SIZE = 4096
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    print("INCORRECT: new byte[] each call")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        for _ in range(N):
            buf = bytearray(BUF_SIZE)
            buf[0] = 1
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
