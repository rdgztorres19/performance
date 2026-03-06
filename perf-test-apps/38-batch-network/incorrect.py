#!/usr/bin/env python3
"""
INCORRECT: One request per item.
Resume: "Batch Network Requests"
Requires: run ../37-connection-pooling/_server.py (port 9999) first.
"""
import socket
import time
HOST = '127.0.0.1'
PORT = 9999
IDS = list(range(100))

def fetch_one(i):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    s.sendall(str(i).encode())
    r = s.recv(1024).decode()
    s.close()
    return r

def main():
    print("INCORRECT: 100 round-trips")
    start = time.perf_counter()
    try:
        results = [fetch_one(i) for i in IDS]
    except ConnectionRefusedError:
        print("Start _server.py first")
        return
    print(f"Fetched {len(IDS)} in {time.perf_counter()-start:.2f}s")

if __name__ == "__main__":
    main()
