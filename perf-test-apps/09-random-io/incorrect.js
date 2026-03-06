const fs = require('fs');
const path = require('path');
const os = require('os');
const SIZE_MB = 20;
const READS = 5000;
const CHUNK = 4096;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const tmp = path.join(os.tmpdir(), 'perf-random-io-' + Date.now() + '.dat');
const buf = Buffer.alloc(SIZE_MB * 1024 * 1024);
buf.fill(0);
fs.writeFileSync(tmp, buf);
let laps = 0;
const start = Date.now();
const maxOff = SIZE_MB * 1024 * 1024 - CHUNK;
const fd = fs.openSync(tmp, 'r');
console.log('INCORRECT: Random seeks');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    fs.closeSync(fd);
    fs.unlinkSync(tmp);
    console.log('Stopped:', laps, 'laps');
    return;
  }
  for (let i = 0; i < READS; i++) {
    const pos = Math.floor(Math.random() * maxOff);
    const b = Buffer.alloc(CHUNK);
    fs.readSync(fd, b, 0, CHUNK, pos);
  }
  laps++;
  setImmediate(run);
}
run();
process.on('SIGINT', () => { fs.closeSync(fd); fs.unlinkSync(tmp); process.exit(0); });
