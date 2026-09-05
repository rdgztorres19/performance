#!/usr/bin/env python3
"""
INCORRECT: One request per item.
Resume: "Batch Network Requests"
Requires: _batch_server.py running (started automatically by docker compose).

Same server and same protocol as correct.py, so the only difference measured
is one round-trip per id versus one round-trip for all of them.
"""
import json
import os
import socket
import time

HOST = os.getenv('SERVER_HOST', '127.0.0.1')
PORT = int(os.getenv('SERVER_PORT', '9998'))
IDS = list(range(100))


def fetch_one(i):
    """One connection and one round-trip for a single id."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))

    # The server reads until the peer half-closes, so send a one-element batch
    # and shut down the write side to signal end of request.
    s.sendall(json.dumps([i]).encode())
    s.shutdown(socket.SHUT_WR)

    data = b''
    while True:
        chunk = s.recv(4096)
        if not chunk:
            break
        data += chunk

    s.close()
    return json.loads(data.decode())


def main():
    print(f"INCORRECT: {len(IDS)} round-trips, one per id", flush=True)

    start = time.perf_counter()
    try:
        results = [fetch_one(i) for i in IDS]
    except ConnectionRefusedError:
        print(f"Cannot reach {HOST}:{PORT}. Start _batch_server.py first.")
        return

    elapsed = time.perf_counter() - start
    print(f"Fetched {len(results)} ids in {elapsed:.3f}s "
          f"({len(IDS)} round-trips)", flush=True)


if __name__ == "__main__":
    main()
