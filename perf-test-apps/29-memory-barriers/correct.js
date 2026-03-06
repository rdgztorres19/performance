/**
 * CORRECT: Atomics - full memory barrier
 */
const { Worker, isMainThread, workerData } = require('worker_threads');
const sab = new SharedArrayBuffer(8);
const view = new Int32Array(sab);
Atomics.store(view, 0, 0);
Atomics.store(view, 1, 0);
if (isMainThread) {
  const w = new Worker(__filename, { workerData: { sab } });
  for (let i = 0; i < 100000; i++) {
    Atomics.store(view, 0, i);
  }
  Atomics.store(view, 1, 1);
  w.on('exit', () => console.log('Stopped'));
} else {
  while (!Atomics.load(view, 1)) {
    Atomics.load(view, 0);
  }
}
