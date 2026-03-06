#!/usr/bin/env python3
"""
INCORRECT: Plain shared vars - possible reordering, visibility issues.
Resume: "Use Memory Barriers"
Note: Python GIL makes this less visible; demo shows the pattern.
"""
import threading
import time
ITERATIONS = 100_000
stop_flag = False
data = 0

def writer():
    global data, stop_flag
    for i in range(ITERATIONS):
        data = i
    stop_flag = True

def reader():
    seen = 0
    while not stop_flag:
        v = data
        if v != seen:
            seen = v
    return seen

def main():
    print("INCORRECT: No memory barrier (visibility not guaranteed)")
    tw = threading.Thread(target=writer)
    tr = threading.Thread(target=reader)
    tr.start()
    tw.start()
    tw.join()
    tr.join(timeout=1)
    print("Stopped")

if __name__ == "__main__":
    main()
