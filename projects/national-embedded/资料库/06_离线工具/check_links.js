const fs = require('fs');
const path = require('path');

const kb = path.resolve(__dirname, '..');
const htmlPath = path.join(kb, 'index.html');
const html = fs.readFileSync(htmlPath, 'utf8');
const refs = [...html.matchAll(/(?:href|src)="([^"]+)"/g)].map((m) => m[1]);
const missing = [];

for (const ref of refs) {
  if (ref.startsWith('#')) continue;
  if (/^[a-z]+:/i.test(ref)) {
    missing.push(`external reference is not allowed: ${ref}`);
    continue;
  }
  const clean = decodeURIComponent(ref.split('#')[0]);
  const target = path.resolve(kb, clean);
  if (!target.startsWith(kb + path.sep) || !fs.existsSync(target)) {
    missing.push(ref);
  }
}

if (missing.length) {
  console.error('Link check failed:');
  for (const item of missing) console.error(` - ${item}`);
  process.exit(1);
}

console.log(`Link check passed: ${refs.length} local references.`);
