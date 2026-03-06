#!/usr/bin/env python3
"""
CORRECT: Reuse connection.
Resume: "Use Connection Pooling"
Requires: run _server.py first.
"""
import socket
import time
HOST = '127.0.0.1'
PORT = 9999
N = 500

def main():
    print("CORRECT: Reuse connection")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print("Start _server.py first: python _server.py")
        return
    start = time.perf_counter()
    for _ in range(N):
        s.sendall(b'ping')
        s.recv(4)
    s.close()
    print(f"{N} requests in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
