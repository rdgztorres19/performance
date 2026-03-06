#!/usr/bin/env python3
"""
INCORRECT: Blocking I/O - threads sit idle waiting.
Resume: "Use Asynchronous I/O"
"""
import concurrent.futures
import os
import signal
import tempfile
import time
NUM_READS = 100
FILE_SIZE_KB = 100
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    print("INCORRECT: Blocking I/O in many threads")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(os.urandom(FILE_SIZE_KB * 1024))
        path = f.name
    try:
        def block_read():
            with open(path, 'rb') as f:
                return len(f.read())
        start = time.perf_counter()
        laps = 0
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            with concurrent.futures.ThreadPoolExecutor(max_workers=NUM_READS) as ex:
                list(ex.map(lambda _: block_read(), range(NUM_READS)))
            laps += 1
        print(f"Stopped: {laps} laps")
    finally:
        os.unlink(path)

if __name__ == "__main__":
    main()
