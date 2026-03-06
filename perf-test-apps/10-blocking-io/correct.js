const fs = require('fs').promises;
const path = require('path');
const os = require('os');
const NUM_READS = 100;
const FILE_KB = 100;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);
const tmp = path.join(os.tmpdir(), 'perf-async-io-' + Date.now() + '.dat');
fs.writeFile(tmp, Buffer.alloc(FILE_KB * 1024)).then(() => {
  let laps = 0;
  const start = Date.now();
  console.log('CORRECT: Async reads');
  function run() {
    if (Date.now() - start >= DURATION_SEC * 1000) {
      fs.unlink(tmp).catch(() => {}).then(() => console.log('Stopped:', laps, 'laps'));
      return;
    }
    const tasks = Array(NUM_READS).fill(null).map(() => fs.readFile(tmp));
    Promise.all(tasks).then(() => {
      laps++;
      setImmediate(run);
    });
  }
  run();
});
process.on('SIGINT', () => process.exit(0));
