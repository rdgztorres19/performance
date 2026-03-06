const N = 100_000;
const BUF = 4096;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
let laps = 0;
const start = Date.now();
console.log('INCORRECT: new Buffer each call');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < N; i++) {
    const buf = Buffer.alloc(BUF);
    buf[0] = 1;
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
