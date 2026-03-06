#!/usr/bin/env python3
"""
INCORRECT: No cancellation - keeps running after "cancel".
Resume: "Use Cancellation Tokens"
"""
import threading
import time
DURATION_SEC = 10
done = False

def long_task():
    for i in range(100):
        if done:
            return
        time.sleep(0.1)

def main():
    global done
    print("INCORRECT: No cooperative cancel (simulate: will run ~10s)")
    t = threading.Thread(target=long_task)
    t.start()
    time.sleep(0.5)
    done = True
    t.join(timeout=1)
    print("Stopped (delay before stop)")

if __name__ == "__main__":
    main()
