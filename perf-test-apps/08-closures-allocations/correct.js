const N = 1_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const data = Array.from({ length: 100 }, (_, i) => i);
const THRESHOLD = 50;
function filterAbove(arr, thresh) {
  let total = 0;
  for (let i = 0; i < arr.length; i++) {
    if (arr[i] > thresh) total += arr[i];
  }
  return total;
}
let laps = 0;
const start = Date.now();
console.log('CORRECT: No closure');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  let total = 0;
  for (let i = 0; i < N; i++) total += filterAbove(data, THRESHOLD);
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
