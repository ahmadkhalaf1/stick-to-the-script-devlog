// Fetches the photographed surfaces the sets are made of, and the night sky over the roof, from Poly Haven
// (polyhaven.com): all CC0, no account needed. Each surface becomes the two pictures Tools/build_materials.py brings in,
// T_<Surface>_C.png (colour, with roughness in its alpha) and T_<Surface>_N.png (bumps), in Saved/Textures. They are
// written over the painted ones of build_textures.mjs, which remain for a surface that is not listed here.
//
//   node Tools/fetch_surfaces.mjs          lists what is missing and how big it is
//   node Tools/fetch_surfaces.mjs --go     downloads it and puts the pictures together (needs ffmpeg)
//
// To change a surface: its id is the last part of its page's address on polyhaven.com.
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, statSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const INCOMING = fileURLToPath(new URL('../Saved/Incoming/PolyHaven/surfaces/', import.meta.url));
const OUT = fileURLToPath(new URL('../Saved/Textures/', import.meta.url));
const AGENT = { 'User-Agent': 'asset-fetch/1.0' };
// 2k for the floors and walls the film gets close to, 1k for the rest.
const WANTED = {
  Concrete: ['concrete_floor_worn_001', '2k'],
  Brick: ['factory_brick', '2k'],
  Floorboards: ['wood_floor_worn', '2k'],
  Wallpaper: ['decrepit_wallpaper', '2k'],
  Roofing: ['tarred_gravel', '2k'],
  Planks: ['wooden_planks', '1k'],
  DarkWood: ['dark_wooden_planks', '1k'],
  Metal: ['corrugated_iron_02', '1k'],
  Steel: ['rusty_metal_04', '1k'],
};
// A sky all the way round as one picture. One with nothing but sky in it: a skyline would bring real buildings and
// their signs into the film.
const SKY = { NightSky: 'kloppenheim_07_puresky' };

async function json(url) {
  const answer = await fetch(url, { headers: AGENT });
  if (!answer.ok) throw new Error(`${url}: ${answer.status}`);
  return answer.json();
}

const go = process.argv.includes('--go');
let total = 0;
let missing = 0;

async function have(name, part) {
  const target = INCOMING + name;
  total += part.size;
  if (existsSync(target) && statSync(target).size === part.size) return target;
  ++missing;
  if (!go) return target;
  const answer = await fetch(part.url, { headers: AGENT });
  if (!answer.ok) throw new Error(`${part.url}: ${answer.status}`);
  writeFileSync(target, Buffer.from(await answer.arrayBuffer()));
  return target;
}

const ffmpeg = (...parts) => execFileSync('ffmpeg', ['-y', '-loglevel', 'error', ...parts]);

mkdirSync(INCOMING, { recursive: true });
mkdirSync(OUT, { recursive: true });
const register = [];
for (const [surface, [id, size]] of Object.entries(WANTED)) {
  const files = await json(`https://api.polyhaven.com/files/${id}`);
  const parts = { diff: files.Diffuse?.[size]?.jpg, rough: files.Rough?.[size]?.jpg, nor_dx: files.nor_dx?.[size]?.jpg };
  const absent = Object.keys(parts).filter(kind => !parts[kind]);
  if (absent.length) throw new Error(`${id} has no ${absent.join(', ')} at ${size}`);
  const before = missing;
  const [colour, rough, bumps] = [await have(`${id}_diff_${size}.jpg`, parts.diff), await have(`${id}_rough_${size}.jpg`, parts.rough), await have(`${id}_nor_dx_${size}.jpg`, parts.nor_dx)];
  if (go) {
    ffmpeg('-i', colour, '-i', rough, '-filter_complex', '[1:v]format=gray[a];[0:v][a]alphamerge', '-frames:v', '1', `${OUT}T_${surface}_C.png`);
    // The bumps of the kind that points down the picture, which is how M_Surface reads them.
    ffmpeg('-i', bumps, '-frames:v', '1', `${OUT}T_${surface}_N.png`);
  }
  const about = await json(`https://api.polyhaven.com/info/${id}`);
  register.push({ surface, id, name: about.name, authors: Object.keys(about.authors ?? {}), size, centimetres: Math.round((about.dimensions?.[0] ?? 0) / 10) });
  console.log(`${surface}: ${id} at ${size}, one copy covers ${Math.round((about.dimensions?.[0] ?? 0) / 10)} cm, ${missing > before ? (go ? 'fetched' : 'to fetch') : 'here already'}`);
}
for (const [name, id] of Object.entries(SKY)) {
  const files = await json(`https://api.polyhaven.com/files/${id}`);
  if (!files.tonemapped) throw new Error(`${id} has no finished picture`);
  const before = missing;
  const picture = await have(`${id}.jpg`, files.tonemapped);
  if (go) {
    // The engine takes no picture wider than 8192. Only the half above the horizon is kept: the set hides the rest.
    ffmpeg('-i', picture, '-vf', 'scale=8192:4096:flags=lanczos,crop=8192:2048:0:0', '-q:v', '2', '-frames:v', '1', `${OUT}T_${name}.jpg`);
  }
  const about = await json(`https://api.polyhaven.com/info/${id}`);
  register.push({ surface: name, id, name: about.name, authors: Object.keys(about.authors ?? {}), size: '8k' });
  console.log(`${name}: ${id}, ${missing > before ? (go ? 'fetched' : 'to fetch') : 'here already'}`);
}
console.log(`${register.length} surfaces, ${(total / 1048576).toFixed(0)} MB${go ? '' : `, ${missing} files to fetch. Nothing was downloaded: add --go.`}`);
if (go) {
  // What came from where, for the asset register and the submission's list.
  writeFileSync(INCOMING + 'fetched.json', JSON.stringify(register, null, 2));
}
