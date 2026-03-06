/**
 * INCORRECT: Adjacent counters in SharedArrayBuffer -> same cache line
 */
const { Worker } = require('worker_threads');
const path = require('path');
const NUM_WORKERS = 4;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const sab = new SharedArrayBuffer(4 * 4);
const stopSab = new SharedArrayBuffer(4);
const stopFlag = new Int32Array(stopSab);
const workerPath = path.join(__dirname, '_worker-shared.js');
const workers = [];
for (let i = 0; i < NUM_WORKERS; i++) {
  workers.push(new Promise((r) => {
    const w = new Worker(workerPath, { workerData: { sab, index: i, stopSab } });
    w.on('exit', r);
  }));
}
console.log('INCORRECT: Adjacent counters -> false sharing');
setTimeout(() => { Atomics.store(stopFlag, 0, 1); Promise.all(workers).then(() => console.log('Stopped')); }, DURATION_SEC * 1000);
process.on('SIGINT', () => Atomics.store(stopFlag, 0, 1));
