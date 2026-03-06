#!/usr/bin/env python3
"""
CORRECT: Single file append - one file, sequential I/O.
Resume: "Avoid Many Small Files"
"""
import os
import signal
import tempfile
import time
NUM_RECORDS = 2000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    fd, path = tempfile.mkstemp(prefix='perf-one-', suffix='.jsonl')
    print("CORRECT: Single JSONL file append")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        with os.fdopen(fd, 'w') as f:
            while not stop and (time.perf_counter() - start) < DURATION_SEC:
                f.seek(0)
                f.truncate()
                for i in range(NUM_RECORDS):
                    f.write('{"id":' + str(i) + '}\n')
                f.flush()
                f.seek(0)
                total = 0
                for line in f:
                    total += len(line)
                laps += 1
    finally:
        os.unlink(path)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
