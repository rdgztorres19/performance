const { workerData } = require('worker_threads');
const { sab, index, stopSab } = workerData;
const view = new Int32Array(sab);
const stop = new Int32Array(stopSab);
while (!Atomics.load(stop, 0)) {
  for (let i = 0; i < 500000; i++) Atomics.add(view, index, 1);
}
