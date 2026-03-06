#!/usr/bin/env python3
"""
INCORRECT: Counters in same cache line - false sharing.
Resume: "Avoid False Sharing"
"""
import multiprocessing as mp
import os
import signal
import time
NUM_PROCS = 4
ITERATIONS = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))
stop = mp.Value('i', 0)

def worker(shared_arr, index, stop_val):
    while stop_val.value == 0:
        for _ in range(ITERATIONS):
            shared_arr[index] = shared_arr[index] + 1

def main():
    print("INCORRECT: 4 counters in same array -> same cache line")
    signal.signal(signal.SIGINT, lambda s, f: setattr(stop, 'value', 1))
    signal.signal(signal.SIGTERM, lambda s, f: setattr(stop, 'value', 1))
    arr = mp.Array('i', [0] * 4)
    procs = [mp.Process(target=worker, args=(arr, i, stop)) for i in range(NUM_PROCS)]
    for p in procs:
        p.start()
    time.sleep(DURATION_SEC)
    stop.value = 1
    for p in procs:
        p.join(timeout=2)
    print("Stopped")

if __name__ == "__main__":
    main()
