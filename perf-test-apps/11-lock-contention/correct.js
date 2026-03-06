/**
 * CORRECT: Atomics.add - lock-free atomic increment
 */
const { Worker, isMainThread, workerData } = require('worker_threads');
const NUM_WORKERS = 8;
const ITERATIONS = 10_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);

if (isMainThread) {
  const sab = new SharedArrayBuffer(8);
  const stopSab = new SharedArrayBuffer(4);
  const arr = new Int32Array(sab);
  console.log('CORRECT: Atomics.add (lock-free)');
  const workers = [];
  for (let i = 0; i < NUM_WORKERS; i++) {
    workers.push(new Promise((r) => {
      const w = new Worker(__filename, { workerData: { sab, stopSab } });
      w.on('exit', r);
    }));
  }
  setTimeout(() => {
    Atomics.store(new Int32Array(stopSab), 0, 1);
    Promise.all(workers).then(() => console.log('Stopped. counter=', arr[0]));
  }, DURATION_SEC * 1000);
  process.on('SIGINT', () => Atomics.store(new Int32Array(stopSab), 0, 1));
} else {
  const { sab, stopSab } = workerData;
  const arr = new Int32Array(sab);
  const stop = new Int32Array(stopSab);
  while (!Atomics.load(stop, 0)) {
    for (let i = 0; i < ITERATIONS; i++) {
      Atomics.add(arr, 0, 1);
    }
  }
}
