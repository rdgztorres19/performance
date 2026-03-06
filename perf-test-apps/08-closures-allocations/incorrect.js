const N = 1_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const data = Array.from({ length: 100 }, (_, i) => i);
let laps = 0;
const start = Date.now();
const threshold = 50;
console.log('INCORRECT: filter with closure');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  let total = 0;
  for (let i = 0; i < N; i++) {
    const filtered = data.filter(x => x > threshold);
    total += filtered.reduce((a, b) => a + b, 0);
  }
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
