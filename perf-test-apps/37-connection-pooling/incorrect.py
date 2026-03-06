#!/usr/bin/env python3
"""
INCORRECT: New connection per request.
Resume: "Use Connection Pooling"
Requires: run _server.py in another terminal first.
"""
import socket
import time
HOST = '127.0.0.1'
PORT = 9999
N = 500

def request():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    s.sendall(b'ping')
    s.recv(4)
    s.close()

def main():
    print("INCORRECT: New socket per request")
    start = time.perf_counter()
    for _ in range(N):
        try:
            request()
        except ConnectionRefusedError:
            print("Start _server.py first: python _server.py")
            return
    print(f"{N} requests in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
