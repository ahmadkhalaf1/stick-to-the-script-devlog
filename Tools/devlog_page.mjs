// Makes the dev log one self-contained web page, pictures included:  node Tools/devlog_page.mjs
// The repository is private, so the log needs a form that can be put anywhere public. The page lands in Saved/Release.
// shortcut: reads only the Markdown the log uses (headings, two levels of bullets, pictures, one table, bold, code).
// A log that starts using more needs a real Markdown reader.
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const lines = readFileSync(join(root, 'DEVLOG.md'), 'utf8').replace(/\r\n/g, '\n').split('\n');

const escape = text => text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const inline = text => escape(text)
  .replace(/`([^`]+)`/g, '<code>$1</code>')
  .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
  .replace(/(^|[\s(])\*([^*\s][^*]*)\*(?=[\s).,;:]|$)/g, '$1<em>$2</em>');

const html = [];
let depth = 0;
let inTable = false;
let pictures = 0;
const listTo = wanted => {
  for (; depth < wanted; ++depth) html.push('<ul>');
  for (; depth > wanted; --depth) html.push('</ul>');
};
const endTable = () => { if (inTable) { html.push('</table>'); inTable = false; } };

for (const line of lines) {
  const bullet = line.match(/^(\s*)- (.*)$/);
  const picture = line.match(/^!\[(.*)\]\((.+)\)$/);
  const heading = line.match(/^(#{1,3}) (.*)$/);

  if (bullet) {
    endTable();
    listTo(bullet[1].length >= 2 ? 2 : 1);
    html.push(`<li>${inline(bullet[2])}</li>`);
    continue;
  }
  if (line.startsWith('|')) {
    listTo(0);
    const cells = line.split('|').slice(1, -1).map(cell => cell.trim());
    if (cells.every(cell => /^-+$/.test(cell))) continue;
    const tag = inTable ? 'td' : 'th';
    if (!inTable) { html.push('<table>'); inTable = true; }
    html.push(`<tr>${cells.map(cell => `<${tag}>${inline(cell)}</${tag}>`).join('')}</tr>`);
    continue;
  }

  listTo(0);
  endTable();
  if (picture) {
    // A picture that is missing stops the build: a broken page must not be the one that gets published.
    const data = readFileSync(join(root, picture[2])).toString('base64');
    html.push(`<figure><img alt="${escape(picture[1])}" src="data:image/jpeg;base64,${data}"><figcaption>${inline(picture[1])}</figcaption></figure>`);
    ++pictures;
  } else if (heading) {
    html.push(`<h${heading[1].length}>${inline(heading[2])}</h${heading[1].length}>`);
  } else if (line === '---') {
    html.push('<hr>');
  } else if (line.trim()) {
    html.push(`<p>${inline(line)}</p>`);
  }
}
listTo(0);
endTable();

const page = `<!doctype html>
<html lang="en">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Stick to the Script: dev log</title>
<style>
  body { font: 17px/1.55 system-ui, sans-serif; color: #1c1c1c; background: #faf8f4; margin: 0 auto; max-width: 820px; padding: 32px 16px 80px; }
  h1 { font-size: 34px; } h2 { margin-top: 56px; border-bottom: 3px solid #d21d1d; padding-bottom: 6px; } h3 { margin-top: 40px; }
  code { background: #ece8df; padding: 1px 5px; border-radius: 4px; font-size: 0.9em; }
  figure { margin: 20px 0; } img { max-width: 100%; display: block; border-radius: 6px; } figcaption { font-size: 14px; color: #666; margin-top: 6px; }
  table { border-collapse: collapse; width: 100%; font-size: 15px; } th, td { border: 1px solid #d8d2c4; padding: 6px 9px; text-align: left; vertical-align: top; }
  hr { border: 0; border-top: 1px solid #d8d2c4; margin: 40px 0; }
  @media (prefers-color-scheme: dark) { body { color: #e9e6df; background: #161514; } code { background: #2b2926; } th, td, hr { border-color: #3a3733; } figcaption { color: #a09a8e; } }
</style>
${html.join('\n')}
</html>
`;

const folder = join(root, 'Saved', 'Release');
mkdirSync(folder, { recursive: true });
writeFileSync(join(folder, 'devlog.html'), page);
console.log(`PASS: ${join(folder, 'devlog.html')} (${(page.length / 1048576).toFixed(1)} MB, ${pictures} pictures, ${html.filter(tag => tag.startsWith('<h3')).length} entries).`);
