#!/usr/bin/env python3
"""Mini TCP server for connection-pooling demo. Run: python _server.py"""
import os
import socket

# Bind to 0.0.0.0 in a container so other containers can reach it.
HOST = os.getenv('HOST', '127.0.0.1')
PORT = int(os.getenv('PORT', '9999'))

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((HOST, PORT))
s.listen(128)
print(f"Server on {HOST}:{PORT}", flush=True)
while True:
    c, _ = s.accept()
    while True:
        d = c.recv(4)
        if not d:
            break
        c.sendall(b'pong')
    c.close()
