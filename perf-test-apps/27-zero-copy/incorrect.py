#!/usr/bin/env python3
"""
INCORRECT: Copy data - allocates new buffer, copies bytes.
Resume: "Use Zero-Copy Patterns"
"""
import os
import signal
import time
N = 1_000_000
CHUNK = 1000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def main():
    global stop
    data = b'x' * CHUNK
    print("INCORRECT: bytearray copy each time")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        for _ in range(N):
            copy = bytearray(data)
            copy[0] = 1
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
