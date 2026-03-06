#!/usr/bin/env python3
"""
INCORRECT: Create/close file per write - metadata overhead.
Resume: "Reduce Filesystem Metadata Operations"
"""
import os
import signal
import tempfile
import time
N = 1000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def main():
    global stop
    tmpdir = tempfile.mkdtemp()
    print("INCORRECT: open/close per write")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    try:
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            for i in range(N):
                path = os.path.join(tmpdir, f"f{i}.tmp")
                with open(path, 'w') as f:
                    f.write('x')
            for i in range(N):
                os.unlink(os.path.join(tmpdir, f"f{i}.tmp"))
            laps += 1
    finally:
        for i in range(N):
            try: os.unlink(os.path.join(tmpdir, f"f{i}.tmp"))
            except OSError: pass
        os.rmdir(tmpdir)
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
