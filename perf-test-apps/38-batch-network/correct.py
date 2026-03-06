#!/usr/bin/env python3
"""
CORRECT: Batch request - one round-trip for many.
Resume: "Batch Network Requests"
Requires: run _batch_server.py first.
"""
import socket
import json
import time
HOST = '127.0.0.1'
PORT = 9998
IDS = list(range(100))

def main():
    print("CORRECT: 1 round-trip for 100 ids")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print("Start _batch_server.py first")
        return
    start = time.perf_counter()
    s.sendall(json.dumps(IDS).encode())
    data = b''
    while len(data) < 1024:
        chunk = s.recv(1024)
        if not chunk:
            break
        data += chunk
    s.close()
    print(f"Batch fetched in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
