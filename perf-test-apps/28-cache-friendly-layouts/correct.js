const N = 500_000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '60', 10);
const ids = Array.from({ length: N }, (_, i) => i);
let laps = 0;
const start = Date.now();
console.log('CORRECT: SoA');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  let total = 0;
  for (let i = 0; i < ids.length; i++) total += ids[i];
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => process.exit(0));
