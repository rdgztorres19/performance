const N = 2_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const data = Array.from({ length: N }, (_, i) => i);
const indices = data.slice().sort(() => Math.random() - 0.5);
let laps = 0, total = 0;
const start = Date.now();
console.log('INCORRECT: Random access');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < indices.length; i++) total += data[indices[i]];
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
