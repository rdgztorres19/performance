#!/usr/bin/env python3
"""
INCORRECT: Unbuffered writes - syscall per write.
Resume: "Use Buffered Streams"
"""
import os
import signal
import tempfile
import time
N = 5000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def main():
    global stop
    fd, path = tempfile.mkstemp()
    print("INCORRECT: Unbuffered (flush per write)")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        with os.fdopen(fd, 'w', buffering=1) as f:
            while not stop and (time.perf_counter() - start) < DURATION_SEC:
                f.seek(0)
                f.truncate()
                for i in range(N):
                    f.write('x')
                    f.flush()
                laps += 1
    finally:
        os.unlink(path)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
