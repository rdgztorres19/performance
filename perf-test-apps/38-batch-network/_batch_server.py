#!/usr/bin/env python3
"""
Batch API server: reads a JSON array of ids and returns one object per id.

Run: python3 _batch_server.py
Environment: HOST (default 127.0.0.1), PORT (default 9998)
"""
import json
import os
import socket

HOST = os.getenv('HOST', '127.0.0.1')
PORT = int(os.getenv('PORT', '9998'))

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((HOST, PORT))
s.listen(128)
print(f"Batch server on {HOST}:{PORT}", flush=True)

while True:
    conn, _ = s.accept()
    try:
        data = b''
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break
            data += chunk

        # A health probe connects and closes without sending anything.
        # Ignore it instead of crashing the server.
        if not data.strip():
            continue

        try:
            ids = json.loads(data.decode())
        except (json.JSONDecodeError, UnicodeDecodeError):
            # Never let one malformed request take the server down.
            continue

        conn.sendall(json.dumps([{"id": i} for i in ids]).encode())
    except (ConnectionResetError, BrokenPipeError):
        pass
    finally:
        conn.close()
