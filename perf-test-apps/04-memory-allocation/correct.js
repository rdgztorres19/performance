const N = 500_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
let laps = 0;
const start = Date.now();
const items = Array.from({ length: N }, () => ({ id: 0, value: '' }));
console.log('CORRECT: Pre-allocate, reuse');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < N; i++) {
    items[i].id = i;
    items[i].value = String(i).repeat(10);
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
