/**
 * INCORRECT: Plain shared array access - no barrier
 *
 * Writing through a plain typed-array element gives no ordering guarantee.
 * The worker may spin on a stale value, which is exactly the bug that memory
 * barriers exist to prevent.
 *
 * A bounded spin limit keeps the demo honest: without it, this program can
 * hang forever, which is the failure it is meant to illustrate.
 */
const { Worker, isMainThread, workerData } = require('worker_threads');

const DATA = 0;
const READY = 1;
const MAX_SPINS = 500_000_000;   // Safety valve so the demo always terminates

if (isMainThread) {
  const sab = new SharedArrayBuffer(8);
  const view = new Int32Array(sab);
  view[DATA] = 0;
  view[READY] = 0;

  console.log('INCORRECT: plain writes -> no ordering guarantee');

  const w = new Worker(__filename, { workerData: { sab } });

  w.on('exit', (code) => console.log(`Stopped (worker exit ${code})`));
  w.on('error', (err) => { console.error('Worker error:', err.message); process.exit(1); });

  for (let i = 0; i < 100000; i++) {
    view[DATA] = i;          // Plain write: no barrier
  }

  view[READY] = 1;           // Plain write: may become visible out of order
} else {
  const view = new Int32Array(workerData.sab);

  let spins = 0;
  // Plain read of the flag: nothing forces this to observe the writer's store.
  while (view[READY] === 0) {
    const v = view[DATA];
    if (++spins > MAX_SPINS) {
      console.log(`Worker gave up after ${spins} spins (flag never observed)`);
      break;
    }
  }

  if (spins <= MAX_SPINS) {
    console.log(`Worker observed the flag after ${spins} spins, data=${view[DATA]}`);
  }
}
