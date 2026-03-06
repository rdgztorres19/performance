#!/usr/bin/env python3
"""
CORRECT: Append only - sequential writes.
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
    print("CORRECT: Append to end")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        with os.fdopen(fd, 'ab') as f:
            while not stop and (time.perf_counter() - start) < DURATION_SEC:
                for i in range(N):
                    f.write(b'x' * 100)  # Append, no seek
                f.flush()
                laps += 1
    finally:
        os.unlink(path)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
