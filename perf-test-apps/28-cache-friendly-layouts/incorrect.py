#!/usr/bin/env python3
"""
INCORRECT: Array of Structs - iterating only one field loads entire struct.
Resume: "Use Cache-Friendly Memory Layouts" - SoA better for single-field.
"""
import os
import signal
import time
N = 500_000
DURATION_SEC = int(os.environ.get("DURATION_SEC", "60"))
stop = False

class Item:
    __slots__ = ('id', 'x', 'y', 'z', 'name')
    def __init__(self, i):
        self.id = i
        self.x = self.y = self.z = i
        self.name = "x"

def main():
    global stop
    data = [Item(i) for i in range(N)]
    print("INCORRECT: AoS - iterating ids loads full struct")
    signal.signal(signal.SIGINT, lambda s, f: globals().__setitem__('stop', True))
    start = time.perf_counter()
    laps = 0
    while not stop and (time.perf_counter() - start) < DURATION_SEC:
        total = 0
        for item in data:
            total += item.id
        laps += 1
    print(f"Stopped: {laps} laps")

if __name__ == "__main__":
    main()
