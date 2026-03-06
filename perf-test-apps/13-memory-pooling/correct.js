const N = 100_000;
const BUF = 4096;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const pool = [];
function rent(size) { return pool.pop() || Buffer.alloc(size); }
function ret(b) { pool.push(b); }
let laps = 0;
const start = Date.now();
console.log('CORRECT: Buffer pool');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < N; i++) {
    const buf = rent(BUF);
    buf[0] = 1;
    ret(buf);
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
