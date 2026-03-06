const fs = require('fs');
const path = require('path');
const os = require('os');
const SIZE_MB = 20;
const CHUNK = 64 * 1024;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const tmp = path.join(os.tmpdir(), 'perf-seq-io-' + Date.now() + '.dat');
const buf = Buffer.alloc(SIZE_MB * 1024 * 1024);
buf.fill(0);
fs.writeFileSync(tmp, buf);
let laps = 0;
const start = Date.now();
const b = Buffer.alloc(CHUNK);
console.log('CORRECT: Sequential reads');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    fs.unlinkSync(tmp);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  const fd = fs.openSync(tmp, 'r');
  while (fs.readSync(fd, b, 0, CHUNK, null) > 0) {}
  fs.closeSync(fd);
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => { try { fs.unlinkSync(tmp); } catch(e){} process.exit(0); });
