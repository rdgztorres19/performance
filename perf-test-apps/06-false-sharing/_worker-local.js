const { workerData } = require('worker_threads');
const { stopSab } = workerData;
const stop = new Int32Array(stopSab);
let localCount = 0;
while (!Atomics.load(stop, 0)) {
  for (let i = 0; i < 500000; i++) localCount++;
}
