const N = 1_000_000;
const CHUNK = 1000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '60', 10);
const data = Buffer.alloc(CHUNK, 'x');
let laps = 0;
const start = Date.now();
console.log('CORRECT: Buffer.slice (view, no copy)');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < N; i++) {
    const view = data.subarray(0, 100);
    data[0] = 1;
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
