#!/usr/bin/env python3
"""Mini TCP server for connection-pooling demo. Run: python _server.py"""
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 9999))
s.listen(5)
print("Server on 127.0.0.1:9999")
while True:
    c, _ = s.accept()
    while True:
        d = c.recv(4)
        if not d:
            break
        c.sendall(b'pong')
    c.close()
