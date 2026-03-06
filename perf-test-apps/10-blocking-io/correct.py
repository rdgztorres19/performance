#!/usr/bin/env python3
"""
CORRECT: Async I/O - single thread, non-blocking.
Resume: "Use Asynchronous I/O"
"""
import asyncio
import os
import signal
import tempfile
import time
NUM_READS = 100
FILE_SIZE_KB = 100
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = False

async def main_async():
    global stop
    print("CORRECT: Async I/O")
    with tempfile.NamedTemporaryFile(delete=False, suffix='.dat') as f:
        f.write(os.urandom(FILE_SIZE_KB * 1024))
        path = f.name
    try:
        start = time.perf_counter()
        laps = 0
        while not stop and (time.perf_counter() - start) < DURATION_SEC:
            def read_file(p):
                with open(p, 'rb') as f:
                    return len(f.read())
            loop = asyncio.get_event_loop()
            tasks = [loop.run_in_executor(None, read_file, path) for _ in range(NUM_READS)]
            await asyncio.gather(*tasks)
            laps += 1
        print(f"Stopped: {laps} laps")
    finally:
        os.unlink(path)

def main():
    global stop
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    signal.signal(signal.SIGTERM, lambda s, f: globals().__setitem__('stop', True))
    asyncio.run(main_async())

if __name__ == "__main__":
    main()
