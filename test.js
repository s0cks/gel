const fs = require('node:fs');

process.nextTick(() => {
  console.log(`test1`);
});

console.log(`hello world`);

process.nextTick(() => {
  console.log(`test2`);
});

console.log(`reading file`);

setTimeout(() => {
  console.log(`timeout`);
  process.nextTick(() => {
    console.log(`next1`);
  })
}, 0);
setTimeout(() => {
  console.log(`timeout2`);
  process.nextTick(() => {
    console.log(`next2`);
  })
}, 0);

fs.readFile('test.js', 'utf8', (err, data) => {
  if(err) {
    console.error(`error: ${err}`);
    return;
  }
  console.log(`data: ${data}`);
});

console.log(`done reading file`);

process.nextTick(() => {
  console.log(`test3`);
});

console.log(`done`);