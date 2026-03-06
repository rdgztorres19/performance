#!/usr/bin/env python3
"""
INCORRECT: JSON - verbose, slower serialize.
Resume: "Use Binary Protocols"
"""
import json
import time
N = 100_000
obj = {"id": 1, "name": "test", "value": 3.14}

def main():
    print("INCORRECT: JSON serialize")
    start = time.perf_counter()
    for _ in range(N):
        json.dumps(obj)
    print(f"{N} serializes in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
