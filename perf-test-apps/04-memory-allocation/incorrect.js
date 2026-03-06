const N = 500_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
let laps = 0;
const start = Date.now();
console.log('INCORRECT: Many allocations');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  const items = [];
  for (let i = 0; i < N; i++) items.push({ id: i, value: String(i).repeat(10) });
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
