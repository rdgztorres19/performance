#!/usr/bin/env python3
"""
INCORRECT: Random I/O - seeks, slow on HDD/SSD.
Resume: "Prefer Sequential I/O"
"""
import os
import random
import signal
import tempfile
import time
SIZE_MB = 20
READS = 5000
CHUNK = 4096
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    print("INCORRECT: Random seeks")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        path = f.name
    try:
        with open(path, 'wb') as f:
            f.write(os.urandom(SIZE_MB * 1024 * 1024))
        max_off = (SIZE_MB * 1024 * 1024) - CHUNK
        start = time.perf_counter()
        laps = 0
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            with open(path, 'rb') as f:
                for _ in range(READS):
                    f.seek(random.randint(0, max_off))
                    f.read(CHUNK)
            laps += 1
        print(f"Stopped: {laps} laps in {time.perf_counter()-start:.1f}s")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
