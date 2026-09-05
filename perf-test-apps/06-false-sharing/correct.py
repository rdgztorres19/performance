#!/usr/bin/env python3
"""
CORRECT: Separate counters per process (no shared array) - no false sharing.
Resume: "Avoid False Sharing" - Per-thread data.
"""
import multiprocessing as mp
import os
import signal
import time
NUM_PROCS = 4
ITERATIONS = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = mp.Value('i', 0)

def worker(index):
    local_count = 0
    while not stop.value:
        for i in range(ITERATIONS):
            local_count += 1
            # Re-check periodically so the worker exits promptly on shutdown.
            if (i & 0xFFFF) == 0 and stop.value:
                break
    return local_count

def main():
    print("CORRECT: Per-process local counter -> no false sharing")
    signal.signal(signal.SIGINT, lambda s, f: setattr(stop, 'value', 1))
    signal.signal(signal.SIGTERM, lambda s, f: setattr(stop, 'value', 1))
    # daemon=True so the parent can always exit, even if a child overruns.
    procs = [mp.Process(target=worker, args=(i,), daemon=True)
             for i in range(NUM_PROCS)]
    for p in procs:
        p.start()
    time.sleep(DURATION_SEC)
    stop.value = 1
    for p in procs:
        p.join(timeout=2)
        if p.is_alive():
            p.terminate()      # Last resort so the demo never hangs
            p.join(timeout=1)
    print("Stopped")

if __name__ == "__main__":
    main()
