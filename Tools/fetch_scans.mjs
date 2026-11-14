// Fetches the scanned models the game uses from Poly Haven (polyhaven.com): photoscanned props, all CC0, no account needed.
// Each lands in Saved/Incoming/PolyHaven/<id>: the model as an FBX, and the textures of its glTF form, which are JPEGs
// with occlusion, roughness and metal packed into one picture. The FBX form comes with far larger ones. Tools/build_scans.py brings them into the project.
//
//   node Tools/fetch_scans.mjs          lists what is missing and how big it is
//   node Tools/fetch_scans.mjs --go     downloads it
//
// To add a model: its id is the last part of its page's address on polyhaven.com. Put it in WANTED with a texture size.
import { existsSync, mkdirSync, writeFileSync, readFileSync, statSync } from 'node:fs';
import { dirname } from 'node:path';

const ROOT = new URL('../Saved/Incoming/PolyHaven/', import.meta.url);
const AGENT = { 'User-Agent': 'asset-fetch/1.0' };
// 2k where the film gets close to it, 1k for the rest.
const WANTED = {
  service_pistol: '2k',
  bolt_action_rifle_7_62: '2k',
  baseball_bat: '1k',
  crowbar_01: '1k',
  sledgehammer_01: '1k',
  ornate_war_hammer: '1k',
  antique_katana_01: '1k',
  wooden_crate_02: '1k',
  brass_pan_01: '1k',
  WetFloorSign_01: '1k',
  bar_chair_round_01: '1k',
  wine_bottles_01: '1k',
  stick_grenade: '1k',
  // What only dresses a set.
  Barrel_01: '1k',
  barrel_03: '1k',
  old_military_crate: '1k',
  wooden_military_crate: '1k',
  cardboard_box_01: '1k',
  steel_frame_shelves_01: '1k',
  old_tyre: '1k',
  metal_tool_chest: '1k',
  korean_fire_extinguisher_01: '1k',
  utility_box_02: '1k',
  utility_box_01: '1k',
  power_box_01: '1k',
  hanging_industrial_lamp: '1k',
  portable_generator: '1k',
  plastic_crate_02: '1k',
  round_wooden_table_01: '1k',
  GreenChair_01: '1k',
  vintage_cabinet_01: '1k',
  ceiling_fan: '1k',
  television_02: '1k',
  standing_chalkboard_01: '1k',
  wine_barrel_01: '1k',
  exterior_aircon_unit: '1k',
  propane_tank: '1k',
  security_camera_01: '1k',
  industrial_wall_lamp: '1k',
  metal_trash_can: '1k',
  concrete_road_barrier: '1k',
};

async function json(url) {
  const answer = await fetch(url, { headers: AGENT });
  if (!answer.ok) throw new Error(`${url}: ${answer.status}`);
  return answer.json();
}

const go = process.argv.includes('--go');
let total = 0;
const register = [];
for (const [id, size] of Object.entries(WANTED)) {
  const folder = new URL(`${id}/`, ROOT);
  const files = await json(`https://api.polyhaven.com/files/${id}`);
  const pack = files.fbx?.[size]?.fbx;
  const pictures = files.gltf?.[size]?.gltf?.include;
  if (!pack || !pictures) throw new Error(`${id} has no FBX or no glTF at ${size}`);
  const parts = [[`${id}.fbx`, pack], ...Object.entries(pictures).filter(([name]) => name.startsWith('textures/'))];
  let bytes = 0;
  let missing = 0;
  for (const [name, part] of parts) {
    bytes += part.size;
    const target = new URL(name, folder);
    if (existsSync(target) && statSync(target).size === part.size) continue;
    ++missing;
    if (!go) continue;
    mkdirSync(dirname(target.pathname.slice(1)), { recursive: true });
    const answer = await fetch(part.url, { headers: AGENT });
    if (!answer.ok) throw new Error(`${part.url}: ${answer.status}`);
    writeFileSync(target, Buffer.from(await answer.arrayBuffer()));
  }
  total += bytes;
  const about = await json(`https://api.polyhaven.com/info/${id}`);
  register.push({ id, name: about.name, authors: Object.keys(about.authors ?? {}), megabytes: +(bytes / 1048576).toFixed(1), textures: size });
  console.log(`${id}: ${(bytes / 1048576).toFixed(1)} MB at ${size}, ${missing ? (go ? `fetched ${missing} files` : `${missing} files to fetch`) : 'here already'}`);
}
console.log(`${Object.keys(WANTED).length} models, ${(total / 1048576).toFixed(0)} MB${go ? '' : '. Nothing was downloaded: add --go.'}`);
if (go) {
  // What came from where, for the asset register and the submission's list.
  writeFileSync(new URL('fetched.json', ROOT), JSON.stringify(register, null, 2));
}
