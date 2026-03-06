/**
 * CORRECT: Each worker has local counter (no shared hot data)
 */
const { Worker } = require('worker_threads');
const path = require('path');
const NUM_WORKERS = 4;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const stopSab = new SharedArrayBuffer(4);
const stopFlag = new Int32Array(stopSab);
const workerPath = path.join(__dirname, '_worker-local.js');
const workers = [];
for (let i = 0; i < NUM_WORKERS; i++) {
  workers.push(new Promise((r) => {
    const w = new Worker(workerPath, { workerData: { stopSab } });
    w.on('exit', r);
  }));
}
console.log('CORRECT: Per-worker local counter');
setTimeout(() => { Atomics.store(stopFlag, 0, 1); Promise.all(workers).then(() => console.log('Stopped')); }, DURATION_SEC * 1000);
process.on('SIGINT', () => Atomics.store(stopFlag, 0, 1));
