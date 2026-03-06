#!/usr/bin/env python3
"""
CORRECT: Reuse buffers from pool.
Resume: "Use Memory Pooling"
"""
import os
import signal
import time
N = 100_000
BUF_SIZE = 4096
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

class SimplePool:
    def __init__(self):
        self.pool = []
    def rent(self, size):
        if self.pool:
            return self.pool.pop()
        return bytearray(size)
    def ret(self, buf):
        self.pool.append(buf)

def main():
    global stop
    pool = SimplePool()
    print("CORRECT: Buffer pool")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        for _ in range(N):
            buf = pool.rent(BUF_SIZE)
            buf[0] = 1
            pool.ret(buf)
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
