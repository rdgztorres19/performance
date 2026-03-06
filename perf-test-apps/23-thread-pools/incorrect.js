const { Worker } = require('worker_threads');
const path = require('path');
const TASKS = 100;
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '60', 10);
const code = `let t=0;for(let i=0;i<1000;i++)t+=i;`;
let laps = 0;
const start = Date.now();
console.log('INCORRECT: new Worker per task');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  const workers = [];
  for (let i = 0; i < TASKS; i++) {
    workers.push(new Promise((r) => {
      const w = new Worker(code, { eval: true });
      w.on('exit', r);
    }));
  }
  Promise.all(workers).then(() => { laps++; setImmediate(run); });
}
run();
process.on('SIGINT', () => process.exit(0));
