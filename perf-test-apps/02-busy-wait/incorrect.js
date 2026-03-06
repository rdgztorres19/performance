#!/usr/bin/env node
/**
 * INCORRECT: Busy-wait loop - burns 100% CPU
 */
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const sab = new SharedArrayBuffer(4);
const stopFlag = new Int32Array(sab);

function busyWait(deadline) {
  while (Date.now() < deadline && !Atomics.load(stopFlag, 0)) {}
}

console.log('INCORRECT: Busy-wait');
const deadline = Date.now() + DURATION_SEC * 1000;
const t = setInterval(() => { Atomics.store(stopFlag, 0, 1); clearInterval(t); console.log('Stopped'); }, DURATION_SEC * 1000);
process.on('SIGINT', () => Atomics.store(stopFlag, 0, 1));
busyWait(deadline);
