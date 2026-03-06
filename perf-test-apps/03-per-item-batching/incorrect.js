const N = 100_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
function processItem(x) { return x * 2 + 1; }
let laps = 0;
const start = Date.now();
console.log('INCORRECT: Per-item');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  let r = 0;
  for (let i = 0; i < N; i++) r += processItem(i);
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
