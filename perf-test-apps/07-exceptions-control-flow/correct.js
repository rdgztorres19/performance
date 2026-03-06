const N = 100_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const DATA = Array(N).fill(null).map((_, i) => ["42","invalid","17","bad","99"][i % 5]);
function parseGood(s) {
  if (/^-?\d+$/.test(s)) return parseInt(s, 10);
  return null;
}
let laps = 0;
const start = Date.now();
console.log('CORRECT: regex check -> no exception');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  let total = 0;
  for (let i = 0; i < DATA.length; i++) {
    const v = parseGood(DATA[i]);
    if (v !== null) total += v;
  }
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
