const fs = require('fs');
const path = require('path');

const kb = path.resolve(__dirname, '..');
const project = path.resolve(kb, '..');
const html = fs.readFileSync(path.join(kb, 'index.html'), 'utf8');
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
  if (!target.startsWith(project + path.sep) || !fs.existsSync(target)) missing.push(ref);
}

if (missing.length) {
  console.error('Link check failed:');
  for (const item of missing) console.error(` - ${item}`);
  process.exit(1);
}
console.log(`Link check passed: ${refs.length} local project references.`);
