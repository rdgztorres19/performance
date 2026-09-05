#!/usr/bin/env python3
"""
INCORRECT: Excessive context switching
Resume: "Reduce Context Switching" - More threads than CPU cores causes
constant rotation, cache eviction, and scheduler overhead.

Run: DURATION_SEC=60 python incorrect.py
Measure: vmstat 1, pidstat -w
"""

import os
import signal
import threading
import multiprocessing
import time

NUM_THREADS = 500  # Way more than typical CPU cores
ITERATIONS = 100_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "300"))

# threading.Event is the right primitive here: workers can test it cheaply and
# the main thread can wait on it with a timeout instead of sleeping blindly.
stop_event = threading.Event()

# Workers wait here until every thread has been created. Without this gate,
# threads that are already spinning starve Thread.start() of the GIL and
# spawning 500 of them takes minutes.
start_gate = threading.Event()


def cpu_busy_work():
    start_gate.wait()
    total = 0
    while not stop_event.is_set():
        for i in range(ITERATIONS):
            total += i * (i + 1)
            # Check often enough that a worker reacts to shutdown quickly.
            # With 500 threads fighting over the GIL, finishing a full batch
            # before checking would make shutdown take minutes.
            if (i & 0x3FF) == 0 and stop_event.is_set():
                return total
    return total


def main():
    num_cores = multiprocessing.cpu_count()
    print(f"INCORRECT: {NUM_THREADS} threads (>> {num_cores} cores) -> context switch storm")
    print(f"Running {DURATION_SEC}s. Measure: vmstat 1, pidstat -w", flush=True)

    signal.signal(signal.SIGINT, lambda s, f: stop_event.set())
    signal.signal(signal.SIGTERM, lambda s, f: stop_event.set())

    # daemon=True means the interpreter can exit even if a worker overruns,
    # so this demo can never hang the container.
    threads = [threading.Thread(target=cpu_busy_work, daemon=True)
               for _ in range(NUM_THREADS)]
    for t in threads:
        t.start()

    # All threads exist and are parked; now let them run together. This is what
    # actually produces the context-switch storm we want to demonstrate.
    start_gate.set()

    # Wait for the duration, but wake immediately on SIGINT/SIGTERM.
    stop_event.wait(timeout=DURATION_SEC)
    stop_event.set()

    # Give the workers a short, BOUNDED window to finish. Joining each thread
    # with its own timeout would add up to NUM_THREADS * timeout in the worst
    # case, so use one shared deadline for all of them.
    deadline = time.monotonic() + 5.0
    for t in threads:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        t.join(timeout=remaining)

    alive = sum(1 for t in threads if t.is_alive())
    if alive:
        print(f"Stopped ({alive} daemon threads still winding down)")
    else:
        print("Stopped")


if __name__ == "__main__":
    main()
