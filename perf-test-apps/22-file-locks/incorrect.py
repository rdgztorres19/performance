#!/usr/bin/env python3
"""
INCORRECT: Exclusive lock - blocks all other access.
Resume: "Avoid File Locks"
"""
import os
import signal
import tempfile
import threading
import time
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

def writer(path):
    while not stop:
        with open(path, 'w') as f:  # Default: exclusive
            f.write('x')
        time.sleep(0.001)

def main():
    global stop
    fd, path = tempfile.mkstemp()
    os.close(fd)
    print("INCORRECT: Exclusive open - blocks readers")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    threads = [threading.Thread(target=writer, args=(path,)) for _ in range(4)]
    for t in threads:
        t.start()
    time.sleep(DURATION_SEC)
    stop = True
    for t in threads:
        t.join()
    os.unlink(path)
    print("Stopped")

if __name__ == "__main__":
    main()
