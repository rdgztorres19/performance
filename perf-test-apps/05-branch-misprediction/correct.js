const N = 5_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const data = Array.from({ length: 100 }, (_, i) => i);
const arr = [];
for (let i = 0; i < N; i++) arr.push(data[i % 100]);
arr.sort((a, b) => (a % 2) - (b % 2));  // Evens first -> predictable
function sumPredictable(a) {
  let total = 0;
  for (let i = 0; i < a.length; i++) {
    if (a[i] % 2 === 0) total += a[i];
    else total -= a[i];
  }
  return total;
}
let laps = 0;
const start = Date.now();
console.log('CORRECT: Sorted -> predictable');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  sumPredictable(arr);
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
