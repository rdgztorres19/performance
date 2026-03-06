#!/usr/bin/env node
/**
 * CORRECT: Limit workers to CPU cores
 * Run: DURATION_SEC=60 node correct.js
 */
const { Worker, isMainThread, workerData } = require('worker_threads');
const os = require('os');

const numWorkers = Math.max(1, (os.cpus().length || 4));
const ITERATIONS = 1_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);

if (isMainThread) {
  const sab = new SharedArrayBuffer(4);
  const stopFlag = new Int32Array(sab);
  console.log('CORRECT:', numWorkers, 'workers (= cores). Running', DURATION_SEC, 's');
  const workers = [];
  for (let i = 0; i < numWorkers; i++) {
    workers.push(new Promise((r) => {
      const w = new Worker(__filename, { workerData: { sab } });
      w.on('exit', r);
    }));
  }
  setTimeout(() => { Atomics.store(stopFlag, 0, 1); Promise.all(workers).then(() => console.log('Stopped')); }, DURATION_SEC * 1000);
  process.on('SIGINT', () => Atomics.store(stopFlag, 0, 1));
} else {
  const { sab } = workerData;
  const stopFlag = new Int32Array(sab);
  while (!Atomics.load(stopFlag, 0)) {
    let total = 0;
    for (let i = 0; i < ITERATIONS; i++) total += i * (i + 1);
  }
}
