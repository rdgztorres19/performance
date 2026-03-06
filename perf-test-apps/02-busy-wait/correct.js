#!/usr/bin/env node
/**
 * CORRECT: Use setInterval/setTimeout - yields to event loop
 */
const DURATION_SEC = parseInt(process.env.DURATION_SEC || '300', 10);

function waitWithYield(deadline) {
  return new Promise((resolve) => {
    const id = setInterval(() => {
      if (Date.now() >= deadline) {
        clearInterval(id);
        resolve();
      }
    }, 10);  // Yield to event loop
  });
}

console.log('CORRECT: setInterval - yields to event loop');
const deadline = Date.now() + DURATION_SEC * 1000;
waitWithYield(deadline).then(() => console.log('Stopped'));
process.on('SIGINT', () => process.exit(0));
