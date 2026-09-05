/**
 * CORRECT: Atomics - full memory barrier
 *
 * Atomics.store/load impose ordering, so when the worker observes the ready
 * flag it is guaranteed to also observe every write that happened before it.
 */
const { Worker, isMainThread, workerData } = require('worker_threads');

const DATA = 0;   // slot holding the value being published
const READY = 1;  // slot holding the "data is published" flag

if (isMainThread) {
  // The main thread owns the buffer and hands it to the worker.
  const sab = new SharedArrayBuffer(8);
  const view = new Int32Array(sab);
  Atomics.store(view, DATA, 0);
  Atomics.store(view, READY, 0);

  console.log('CORRECT: Atomics store/load -> ordered, worker always sees the flag');

  const w = new Worker(__filename, { workerData: { sab } });

  w.on('exit', (code) => console.log(`Stopped (worker exit ${code})`));
  w.on('error', (err) => { console.error('Worker error:', err.message); process.exit(1); });

  for (let i = 0; i < 100000; i++) {
    Atomics.store(view, DATA, i);
  }

  // Release: publishing the flag with Atomics guarantees the writes above are
  // visible to any thread that observes it.
  Atomics.store(view, READY, 1);
  Atomics.notify(view, READY);
} else {
  // Use the buffer we were given. Creating a new one here would mean the two
  // threads never share memory and the flag would never arrive.
  const view = new Int32Array(workerData.sab);

  let spins = 0;
  while (Atomics.load(view, READY) === 0) {
    Atomics.load(view, DATA);
    spins++;
  }

  console.log(`Worker observed the flag after ${spins} spins, data=${Atomics.load(view, DATA)}`);
}
