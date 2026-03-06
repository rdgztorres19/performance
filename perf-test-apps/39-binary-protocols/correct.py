#!/usr/bin/env python3
"""
CORRECT: struct.pack - compact binary.
Resume: "Use Binary Protocols"
"""
import struct
import time
N = 100_000

def pack_item(i, name, v):
    return struct.pack('Id8sd', i, v, name.encode()[:8].ljust(8), v)

def main():
    print("CORRECT: struct.pack binary")
    start = time.perf_counter()
    for i in range(N):
        pack_item(1, "test", 3.14)
    print(f"{N} serializes in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
