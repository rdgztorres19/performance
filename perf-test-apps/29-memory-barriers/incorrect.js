/**
 * INCORRECT: Plain shared var - no barrier
 */
const { Worker, isMainThread, workerData } = require('worker_threads');
const sab = new SharedArrayBuffer(8);
const view = new Int32Array(sab);
view[0] = 0;
view[1] = 0;
if (isMainThread) {
  const w = new Worker(__filename, { workerData: { sab } });
  for (let i = 0; i < 100000; i++) {
    view[0] = i;
  }
  view[1] = 1;
  w.on('exit', () => console.log('Stopped'));
} else {
  while (!Atomics.load(view, 1)) {
    const v = view[0];
  }
}
