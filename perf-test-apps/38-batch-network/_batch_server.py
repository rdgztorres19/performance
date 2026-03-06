#!/usr/bin/env python3
"""Batch API server. Run: python _batch_server.py"""
import socket
import json
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 9998))
s.listen(5)
print("Batch server on 127.0.0.1:9998")
while True:
    c, _ = s.accept()
    data = b''
    while True:
        chunk = c.recv(4096)
        if not chunk:
            break
        data += chunk
    ids = json.loads(data.decode())
    c.sendall(json.dumps([{"id": i} for i in ids]).encode())
    c.close()
