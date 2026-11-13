// Paints the surfaces the sets are built from: for each one a colour picture with roughness in its alpha, and a normal map.
// The textures are generated, so this script is their source.
//
//   node Tools/build_textures.mjs
//
// It writes PNG files to Saved/Textures. Tools/build_materials.py then brings them into Content/Materials.

import { mkdirSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { crc32, deflateSync } from 'node:zlib';

const SIZE = 1024;
const OUT = fileURLToPath(new URL('../Saved/Textures/', import.meta.url));

const mix = (a, b, t) => a + (b - a) * t;
const mix3 = (a, b, t) => [mix(a[0], b[0], t), mix(a[1], b[1], t), mix(a[2], b[2], t)];
const times = (colour, k) => [colour[0] * k, colour[1] * k, colour[2] * k];
const fract = x => x - Math.floor(x);
const wrap = (n, period) => ((n % period) + period) % period;
const smooth = (from, to, x) => {
  const t = Math.min(Math.max((x - from) / (to - from), 0), 1);
  return t * t * (3 - 2 * t);
};
// How far a point is from the nearest edge of the cell it is in, as a share of the cell: 0 on an edge, 0.5 in the middle.
const fromEdge = x => 0.5 - Math.abs(fract(x) - 0.5);

function hash(x, y, seed) {
  let h = Math.imul(x | 0, 374761393) + Math.imul(y | 0, 668265263) + Math.imul(seed | 0, 1442695041);
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  h ^= h >>> 16;
  return (h >>> 0) / 4294967296;
}

// Repeats after periodX by periodY cells, which is what lets every texture tile.
function noise(x, y, periodX, periodY, seed) {
  const x0 = Math.floor(x), y0 = Math.floor(y);
  const fx = x - x0, fy = y - y0;
  const sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
  const a = hash(wrap(x0, periodX), wrap(y0, periodY), seed);
  const b = hash(wrap(x0 + 1, periodX), wrap(y0, periodY), seed);
  const c = hash(wrap(x0, periodX), wrap(y0 + 1, periodY), seed);
  const d = hash(wrap(x0 + 1, periodX), wrap(y0 + 1, periodY), seed);
  return mix(mix(a, b, sx), mix(c, d, sx), sy);
}

// Layers of noise, each twice as fine as the one before. u and v run from 0 to 1 across the texture.
function cloud(u, v, cellsX, cellsY, layers, seed) {
  let sum = 0, weight = 0.5, total = 0;
  for (let layer = 0; layer < layers; layer++) {
    sum += weight * noise(u * cellsX, v * cellsY, cellsX, cellsY, seed + layer * 17);
    total += weight;
    cellsX *= 2;
    cellsY *= 2;
    weight *= 0.5;
  }
  return sum / total;
}

// A line one or two pixels wide where slow noise crosses its middle value. Dividing by the slope keeps the width even;
// without it the line swells into a blot wherever the noise is nearly flat.
function hairline(u, v, seed) {
  const STEP = 1 / SIZE;
  const here = cloud(u, v, 3, 3, 5, seed);
  const slope = Math.hypot(cloud(u + STEP, v, 3, 3, 5, seed) - here, cloud(u, v + STEP, 3, 3, 5, seed) - here);
  return 1 - smooth(0.5, 1.6, Math.abs(here - 0.5) / Math.max(slope, 1e-6));
}

// Small round dots, at most one to a cell of the grid and never touching its edge. chance is how many cells get one.
function dots(u, v, cells, chance, biggest, seed) {
  const cellX = Math.floor(u * cells), cellY = Math.floor(v * cells);
  if (hash(cellX, cellY, seed) > chance) {
    return 0;
  }
  const centreX = 0.3 + hash(cellX, cellY, seed + 1) * 0.4, centreY = 0.3 + hash(cellX, cellY, seed + 2) * 0.4;
  const radius = biggest * (0.35 + hash(cellX, cellY, seed + 3) * 0.65);
  return 1 - smooth(radius * 0.5, radius, Math.hypot(u * cells - cellX - centreX, v * cells - cellY - centreY));
}

// Every surface answers, for a point: its colour as seen on a screen, how rough it is, and how high it stands.
const surfaces = {
  // A poured floor in slabs of half a tile, with worn patches that shine and a few pits.
  Concrete: {
    relief: 2.2,
    at(u, v) {
      const mottle = cloud(u, v, 4, 4, 5, 11);
      const speckle = cloud(u, v, 64, 64, 3, 23);
      const worn = smooth(0.5, 0.66, cloud(u, v, 3, 3, 4, 37));
      const joint = 1 - smooth(1.0, 3.0, Math.min(fromEdge(u * 2), fromEdge(v * 2)) * SIZE / 2);
      const pit = dots(u, v, 96, 0.13, 0.22, 51);
      const crack = hairline(u, v, 91) * smooth(0.54, 0.66, cloud(u, v, 2, 2, 3, 57));
      const tone = 0.5 + (mottle - 0.5) * 0.24 + (speckle - 0.5) * 0.12 - worn * 0.07 - joint * 0.16 - pit * 0.14 - crack * 0.2;
      return {
        colour: [tone, tone * 0.99, tone * 0.96],
        rough: 0.9 - worn * 0.42 + (speckle - 0.5) * 0.12,
        height: 0.5 + (speckle - 0.5) * 0.22 + (mottle - 0.5) * 0.3 - joint * 0.9 - pit * 0.35 - crack * 0.6,
      };
    },
  },

  // Eight bricks across and thirty-two courses up, each brick its own shade, the mortar set back.
  Brick: {
    relief: 4.5,
    at(u, v) {
      const COLUMNS = 8, ROWS = 32;
      const row = Math.floor(v * ROWS);
      const along = u * COLUMNS + (row % 2) * 0.5;
      const column = wrap(Math.floor(along), COLUMNS);
      const ragged = (cloud(u, v, 48, 48, 2, 5) - 0.5) * 3;
      const toEdge = Math.min(fromEdge(along) * SIZE / COLUMNS, fromEdge(v * ROWS) * SIZE / ROWS);
      const face = smooth(2.5, 5.5, toEdge + ragged);
      const shade = hash(column, row, 71), kind = hash(column, row, 72);
      const grain = cloud(u, v, 96, 96, 3, 9);
      let clay = mix3([0.56, 0.28, 0.2], [0.43, 0.21, 0.17], shade);
      if (kind > 0.86) clay = [0.3, 0.18, 0.16];
      if (kind < 0.1) clay = [0.63, 0.4, 0.31];
      const sand = cloud(u, v, 128, 128, 2, 13);
      const mortar = times([0.55, 0.53, 0.5], 0.82 + sand * 0.32);
      return {
        colour: mix3(mortar, times(clay, 0.82 + grain * 0.36), face),
        rough: mix(0.96, 0.8 + (grain - 0.5) * 0.2, face),
        height: mix(sand * 0.15, 0.78 + (grain - 0.5) * 0.3 + (shade - 0.5) * 0.14, face),
      };
    },
  },

  // Sixteen boards, running along u, each cut from a different tree, with a dark gap between them.
  Planks: {
    relief: 2.6,
    at(u, v) {
      const BOARDS = 16;
      const board = Math.floor(v * BOARDS);
      const slid = fract(u + hash(board, 0, 31));
      const piece = Math.floor(slid * 2);
      const tree = hash(board, piece, 33);
      const toEdge = Math.min(fromEdge(v * BOARDS) * SIZE / BOARDS, fromEdge(slid * 2) * SIZE / 2);
      const gap = 1 - smooth(1.0, 3.2, toEdge);
      const seed = 100 + board * 2 + piece;
      const grain = cloud(u, v, 8, 256, 3, seed);
      const rings = 0.5 + 0.5 * Math.sin((v * BOARDS * 3 + grain * 5 + tree * 9) * Math.PI * 2);
      const cupped = Math.pow(Math.abs(fract(v * BOARDS) - 0.5) * 2, 4);
      const tone = (0.78 + (tree - 0.5) * 0.4) * (0.8 + grain * 0.28 + rings * 0.12) * (1 - gap * 0.78);
      return {
        colour: times([0.52, 0.34, 0.2], tone),
        rough: 0.46 + grain * 0.22 + gap * 0.4,
        height: 0.6 + (grain - 0.5) * 0.2 + rings * 0.06 - cupped * 0.14 - gap * 0.9,
      };
    },
  },

  // Striped paper with a row of diamonds down each stripe, slightly raised and shinier than the paper.
  Wallpaper: {
    relief: 1.2,
    at(u, v) {
      const STRIPES = 8;
      // Half a stripe along, so the edge of the tile falls in the middle of a stripe and not on a change of shade.
      const across = u * STRIPES + 0.5;
      const stripe = Math.floor(across) % 2;
      const acrossMotif = fract(across) - 0.5;
      const alongMotif = fract(v * STRIPES + stripe * 0.5) - 0.5;
      const diamond = 1 - smooth(0.2, 0.24, Math.abs(acrossMotif) + Math.abs(alongMotif));
      const dot = 1 - smooth(0.05, 0.08, Math.abs(acrossMotif) + Math.abs(alongMotif));
      const seam = 1 - smooth(0.4, 1.6, fromEdge(across) * SIZE / STRIPES);
      const fleck = cloud(u, v, 128, 128, 2, 41);
      const faded = cloud(u, v, 3, 3, 3, 43);
      const tone = (stripe ? 0.64 : 0.5) + (diamond - dot) * 0.16 - seam * 0.1 + (faded - 0.5) * 0.14;
      return {
        colour: times([tone, tone * 0.93, tone * 0.86], 0.92 + fleck * 0.16),
        rough: 0.8 - (diamond - dot) * 0.3,
        height: 0.5 + fleck * 0.08 + (diamond - dot) * 0.2 - seam * 0.15,
      };
    },
  },

  // Roofing felt under grit, laid in three strips to a tile, with patches of bare tar.
  Roofing: {
    relief: 3.2,
    at(u, v) {
      const grit = noise(u * 256, v * 256, 256, 256, 61);
      const pebble = Math.max(dots(u, v, 120, 0.5, 0.3, 63), dots(u, v, 73, 0.3, 0.3, 69));
      const mottle = cloud(u, v, 4, 4, 4, 65);
      const tar = smooth(0.6, 0.72, cloud(u, v, 9, 9, 3, 67)) * 0.6;
      const seam = 1 - smooth(1.0, 4.0, fromEdge(v * 3) * SIZE / 3);
      const tone = (0.32 + (mottle - 0.5) * 0.14 + (grit - 0.5) * 0.16 + pebble * 0.14) * (1 - tar * 0.45) - seam * 0.1;
      return {
        colour: [tone * 0.95, tone * 0.97, tone * 1.05],
        rough: 0.93 - tar * 0.45,
        height: 0.4 + (grit * 0.3 + pebble * 0.5) * (1 - tar * 0.8) - seam * 0.5,
      };
    },
  },

  // Corrugated sheet, sixteen ribs to a tile running along v, with rust in streaks down it.
  Metal: {
    relief: 7.0,
    at(u, v) {
      const rib = 0.5 + 0.5 * Math.cos(u * 16 * Math.PI * 2);
      const rust = smooth(0.58, 0.74, cloud(u, v, 24, 3, 4, 81) * 0.75 + cloud(u, v, 6, 6, 3, 83) * 0.25);
      const scratch = cloud(u, v, 256, 4, 2, 85);
      const seam = 1 - smooth(1.0, 3.5, fromEdge(v) * SIZE);
      const zinc = times([0.58, 0.6, 0.63], 0.86 + scratch * 0.22);
      const rusty = times([0.42, 0.22, 0.12], 0.75 + scratch * 0.5);
      return {
        colour: times(mix3(zinc, rusty, rust), 1 - seam * 0.5),
        rough: mix(0.36 + scratch * 0.12, 0.88, rust),
        height: rib * 0.9 + rust * 0.04 + (scratch - 0.5) * 0.03 - seam * 0.3,
      };
    },
  },
};

function png(pixels, channels) {
  const stride = SIZE * channels;
  const raw = Buffer.alloc((stride + 1) * SIZE);
  for (let y = 0; y < SIZE; y++) {
    pixels.copy(raw, y * (stride + 1) + 1, y * stride, (y + 1) * stride);
  }
  const chunk = (type, data) => {
    const body = Buffer.concat([Buffer.from(type, 'latin1'), data]);
    const length = Buffer.alloc(4), check = Buffer.alloc(4);
    length.writeUInt32BE(data.length);
    check.writeUInt32BE(crc32(body) >>> 0);
    return Buffer.concat([length, body, check]);
  };
  const header = Buffer.alloc(13);
  header.writeUInt32BE(SIZE, 0);
  header.writeUInt32BE(SIZE, 4);
  header[8] = 8;
  header[9] = channels === 4 ? 6 : 2;
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', header), chunk('IDAT', deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}

const byte = x => Math.round(Math.min(Math.max(x, 0), 1) * 255);

function paint(name, surface) {
  const colour = Buffer.alloc(SIZE * SIZE * 4);
  const height = new Float32Array(SIZE * SIZE);
  for (let y = 0; y < SIZE; y++) {
    for (let x = 0; x < SIZE; x++) {
      const point = surface.at((x + 0.5) / SIZE, (y + 0.5) / SIZE);
      const index = y * SIZE + x;
      colour[index * 4] = byte(point.colour[0]);
      colour[index * 4 + 1] = byte(point.colour[1]);
      colour[index * 4 + 2] = byte(point.colour[2]);
      colour[index * 4 + 3] = byte(point.rough);
      height[index] = point.height;
    }
  }

  // The normal leans away from a rise: x along u, y along v, z out of the surface. The material lays it into the world.
  const normal = Buffer.alloc(SIZE * SIZE * 3);
  const high = (x, y) => height[wrap(y, SIZE) * SIZE + wrap(x, SIZE)];
  for (let y = 0; y < SIZE; y++) {
    for (let x = 0; x < SIZE; x++) {
      const dx = (high(x - 1, y) - high(x + 1, y)) * surface.relief;
      const dy = (high(x, y - 1) - high(x, y + 1)) * surface.relief;
      const length = Math.hypot(dx, dy, 1);
      const index = (y * SIZE + x) * 3;
      normal[index] = byte(dx / length * 0.5 + 0.5);
      normal[index + 1] = byte(dy / length * 0.5 + 0.5);
      normal[index + 2] = byte(1 / length * 0.5 + 0.5);
    }
  }

  writeFileSync(`${OUT}T_${name}_C.png`, png(colour, 4));
  writeFileSync(`${OUT}T_${name}_N.png`, png(normal, 3));
}

// The two edges of a tiling texture must meet. This is the one thing here that is easy to break without seeing it.
function checkItTiles(name, surface) {
  let worst = 0;
  for (let step = 0; step < SIZE; step += 7) {
    const along = (step + 0.5) / SIZE;
    for (const [near, far] of [[surface.at(0.0001, along), surface.at(0.9999, along)], [surface.at(along, 0.0001), surface.at(along, 0.9999)]]) {
      worst = Math.max(worst, Math.abs(near.height - far.height), Math.abs(near.colour[0] - far.colour[0]));
    }
  }
  if (worst > 0.12) {
    throw new Error(`${name} does not tile: its edges differ by ${worst.toFixed(2)}`);
  }
}

mkdirSync(OUT, { recursive: true });
for (const [name, surface] of Object.entries(surfaces)) {
  checkItTiles(name, surface);
  paint(name, surface);
  console.log(`painted T_${name}`);
}
