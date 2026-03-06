#!/usr/bin/env python3
"""
CORRECT: Use threading primitives that imply barriers (lock, Event).
Resume: "Use Memory Barriers" - Lock acquire/release has barrier semantics.
"""
import threading
import time
ITERATIONS = 100_000
lock = threading.Lock()
data = 0
stop_flag = False

def writer():
    global data, stop_flag
    for i in range(ITERATIONS):
        with lock:
            data = i
    with lock:
        stop_flag = True

def reader():
    seen = 0
    while True:
        with lock:
            if stop_flag:
                break
            v = data
        if v != seen:
            seen = v
    return seen

def main():
    print("CORRECT: Lock (implies memory barrier)")
    tw = threading.Thread(target=writer)
    tr = threading.Thread(target=reader)
    tr.start()
    tw.start()
    tw.join()
    tr.join(timeout=2)
    print("Stopped")

if __name__ == "__main__":
    main()
