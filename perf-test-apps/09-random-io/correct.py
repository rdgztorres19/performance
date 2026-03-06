#!/usr/bin/env python3
"""
CORRECT: Sequential I/O - enables prefetch, efficient.
Resume: "Prefer Sequential I/O"
"""
import os
import signal
import tempfile
import time
SIZE_MB = 20
CHUNK = 64 * 1024  # 64KB
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    print("CORRECT: Sequential reads")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        path = f.name
    try:
        with open(path, 'wb') as f:
            f.write(os.urandom(SIZE_MB * 1024 * 1024))
        start = time.perf_counter()
        laps = 0
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            with open(path, 'rb') as f:
                while f.read(CHUNK):
                    pass
            laps += 1
        print(f"Stopped: {laps} laps in {time.perf_counter()-start:.1f}s")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
