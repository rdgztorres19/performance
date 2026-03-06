#!/usr/bin/env python3
"""
CORRECT: Check cancellation token frequently.
Resume: "Use Cancellation Tokens"
"""
import threading
import time
cancel = threading.Event()

def long_task():
    for i in range(100):
        if cancel.is_set():
            return
        time.sleep(0.01)

def main():
    print("CORRECT: Check cancel token each iteration")
    t = threading.Thread(target=long_task)
    t.start()
    time.sleep(0.05)
    cancel.set()
    t.join(timeout=0.5)
    print("Stopped (fast response)")

if __name__ == "__main__":
    main()
