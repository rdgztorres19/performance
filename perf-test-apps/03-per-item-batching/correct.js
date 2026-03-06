const N = 100_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
function processBatch(data) {
  let total = 0;
  for (let i = 0; i < data.length; i++) total += data[i] * 2 + 1;
  return total;
}
const data = Array.from({ length: N }, (_, i) => i);
let laps = 0;
const start = Date.now();
console.log('CORRECT: Batch');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  processBatch(data);
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
