#!/usr/bin/env python3
"""
CORRECT: Batch request - one round-trip for many.
Resume: "Batch Network Requests"
Requires: run _batch_server.py first.
"""
import os
import socket
import json
import time
HOST = os.getenv('SERVER_HOST', '127.0.0.1')
PORT = int(os.getenv('SERVER_PORT', '9998'))
IDS = list(range(100))

def main():
    print(f"CORRECT: 1 round-trip for {len(IDS)} ids", flush=True)
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print(f"Cannot reach {HOST}:{PORT}. Start _batch_server.py first.")
        return
    start = time.perf_counter()
    s.sendall(json.dumps(IDS).encode())
    s.shutdown(socket.SHUT_WR)      # Tell the server the request is complete

    data = b''
    while True:
        chunk = s.recv(4096)
        if not chunk:
            break
        data += chunk
    s.close()

    results = json.loads(data.decode())
    elapsed = time.perf_counter() - start
    print(f"Fetched {len(results)} ids in {elapsed:.3f}s (1 round-trip)", flush=True)

if __name__ == "__main__":
    main()
