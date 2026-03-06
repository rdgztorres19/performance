#!/usr/bin/env python3
"""
INCORRECT: Update in place - random writes, seeks.
Resume: "Use Append-Only Storage"
"""
import os
import signal
import tempfile
import time
N = 1000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    fd, path = tempfile.mkstemp()
    REC_SIZE = 100
    with os.fdopen(fd, 'wb') as f:
        f.write(b'\x00' * N * REC_SIZE)
    print("INCORRECT: seek + write per update")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        with open(path, 'r+b') as f:
            while not stop and (time.perf_counter() - start) < DURATION_SEC:
                for i in range(N):
                    f.seek(i * REC_SIZE)
                    f.write(b'x' * REC_SIZE)
                laps += 1
    finally:
        os.unlink(path)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
