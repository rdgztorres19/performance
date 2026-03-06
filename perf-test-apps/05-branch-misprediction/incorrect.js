const N = 5_000_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const data = Array.from({ length: N }, () => Math.floor(Math.random() * 100));
function sumUnpredictable(arr) {
  let total = 0;
  for (let i = 0; i < arr.length; i++) {
    if (arr[i] % 2 === 0) total += arr[i];
    else total -= arr[i];
  }
  return total;
}
let laps = 0;
const start = Date.now();
console.log('INCORRECT: Unpredictable branches');
const id = setInterval(() => {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    clearInterval(id);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  sumUnpredictable(data);
  laps++;
}, 0);
process.on('SIGINT', () => { clearInterval(id); process.exit(0); });
