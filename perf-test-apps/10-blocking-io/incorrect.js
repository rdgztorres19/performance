const fs = require('fs');
const path = require('path');
const os = require('os');
const NUM_READS = 100;
const FILE_KB = 100;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const tmp = path.join(os.tmpdir(), 'perf-block-io-' + Date.now() + '.dat');
fs.writeFileSync(tmp, Buffer.alloc(FILE_KB * 1024));
let laps = 0;
const start = Date.now();
console.log('INCORRECT: Blocking sync reads in loop');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    fs.unlinkSync(tmp);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < NUM_READS; i++) {
    fs.readFileSync(tmp);
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => { try { fs.unlinkSync(tmp); } catch(e){} process.exit(0); });
