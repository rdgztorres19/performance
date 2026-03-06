const { Worker } = require('worker_threads');
const NUM_WORKERS = 8;  // Pool size = cores
const TASKS_PER_WORKER = 13;  // 8*13 ~ 100 tasks
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '60', 10);
let laps = 0;
const start = Date.now();
const code = `let t=0;for(let i=0;i<${1000*TASKS_PER_WORKER};i++)t+=i;`;
console.log('CORRECT: 8 workers (pool) vs 100');
function run() {
  if (Date.now() - start >= DURATION_SEC * 1000) {
    console.log('Stopped:', laps, 'laps');
    return;
  }
  const workers = [];
  for (let i = 0; i < NUM_WORKERS; i++) {
    workers.push(new Promise((r) => {
      const w = new Worker(code, { eval: true });
      w.on('exit', r);
    }));
  }
  Promise.all(workers).then(() => { laps++; setImmediate(run); });
}
run();
process.on('SIGINT', () => process.exit(0));
