const fs = require('fs');
const path = require('path');
const os = require('os');
const NUM_RECORDS = 2000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const filepath = path.join(os.tmpdir(), 'perf-one-' + Date.now() + '.jsonl');
let laps = 0;
const start = Date.now();
console.log('CORRECT: Single JSONL file');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    fs.unlinkSync(filepath);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  const lines = [];
  for (let i = 0; i < NUM_RECORDS; i++) lines.push('{"id":' + i + '}');
  fs.writeFileSync(filepath, lines.join('\n'));
  const data = fs.readFileSync(filepath, 'utf8');
  data.split('\n').forEach(() => {});
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => { try { fs.unlinkSync(filepath); } catch(e){} process.exit(0); });
