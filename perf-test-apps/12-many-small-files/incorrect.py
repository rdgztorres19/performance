#!/usr/bin/env python3
"""
INCORRECT: Many small files - metadata overhead, inode pressure.
Resume: "Avoid Many Small Files"
"""
import os
import signal
import tempfile
import time
NUM_FILES = 2000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

def main():
    global stop
    tmpdir = tempfile.mkdtemp(prefix='perf-many-')
    print("INCORRECT: 2000 files create/read/delete per iter")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            for i in range(NUM_FILES):
                p = os.path.join(tmpdir, f"{i}.json")
                with open(p, 'w') as f:
                    f.write('{"id":' + str(i) + '}')
            for i in range(NUM_FILES):
                p = os.path.join(tmpdir, f"{i}.json")
                with open(p, 'r') as f:
                    f.read()
            for i in range(NUM_FILES):
                os.unlink(os.path.join(tmpdir, f"{i}.json"))
            laps += 1
    finally:
        for i in range(NUM_FILES):
            try:
                os.unlink(os.path.join(tmpdir, f"{i}.json"))
            except OSError:
                pass
        os.rmdir(tmpdir)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
