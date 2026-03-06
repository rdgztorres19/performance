const fs = require('fs');
const path = require('path');
const os = require('os');
const NUM_FILES = 2000;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const tmpdir = fs.mkdtempSync(path.join(os.tmpdir(), 'perf-many-'));
let laps = 0;
const start = Date.now();
console.log('INCORRECT: 2000 files/iter');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    for (let i = 0; i < NUM_FILES; i++) try { fs.unlinkSync(path.join(tmpdir, i + '.json')); } catch(e){}
    fs.rmdirSync(tmpdir);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < NUM_FILES; i++) {
    fs.writeFileSync(path.join(tmpdir, i + '.json'), '{"id":' + i + '}');
  }
  for (let i = 0; i < NUM_FILES; i++) {
    fs.readFileSync(path.join(tmpdir, i + '.json'));
  }
  for (let i = 0; i < NUM_FILES; i++) {
    fs.unlinkSync(path.join(tmpdir, i + '.json'));
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => { try { fs.rmdirSync(tmpdir, { recursive: true }); } catch(e){} process.exit(0); });
