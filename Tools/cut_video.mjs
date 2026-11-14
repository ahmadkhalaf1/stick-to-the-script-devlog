// Cuts the gameplay video from what Tools\record_run.ps1 recorded: run.mp4 (picture), run.wav (the game's own sound)
// and run.log, all in Saved/Release/video. The log says when everything happened, so the script picks the moments
// itself: the title, the best blows of each fight, the wrap, the choice of cut and some of the film.
//
//   node Tools/cut_video.mjs
//
// It writes Saved/Release/StickToTheScript_gameplay.mp4, under a minute, and a sheet of frames to look at.

import { execFileSync, spawnSync } from 'node:child_process';
import { openSync, readFileSync, readSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const DIR = fileURLToPath(new URL('../Saved/Release/video/', import.meta.url));
const OUT = fileURLToPath(new URL('../Saved/Release/StickToTheScript_gameplay.mp4', import.meta.url));
const LONGEST = 59;
// What a blow is worth as a moment to show. A knockout adds to whatever landed it.
const WORTH = { SwingKick: 10, Throw: 8, Roundhouse: 7, Uppercut: 7, Sweep: 5, Chop: 4, Kick: 2, Punch: 1 };
const KNOCKOUT = 5;
const HIGHLIGHTS = [4, 3, 3];

// Every line of the log starts with the moment it was written, to the millisecond.
const log = readFileSync(`${DIR}run.log`, 'utf8').split('\n').map(line => {
  const stamp = line.match(/^\[(\d+)\.(\d+)\.(\d+)-(\d+)\.(\d+)\.(\d+):(\d+)\]/);
  return stamp ? { at: Date.UTC(stamp[1], stamp[2] - 1, stamp[3], stamp[4], stamp[5], stamp[6], stamp[7]) / 1000, text: line } : null;
}).filter(Boolean);
const first = text => log.find(line => line.text.includes(text));
const all = text => log.filter(line => line.text.includes(text));

const soundStarted = first('Sound recording started');
if (!soundStarted) throw new Error('The log has no "Sound recording started": the game was not run with -RecordSound');

// Films, as the log saw them, and as the picture shows them: the top of the frame goes black for exactly as long.
const films = all('OnFinalCutStarted').map(start => ({ from: start.at, to: (log.find(line => line.at > start.at && line.text.includes('OnFinalCutFinished')) ?? {}).at }))
  .filter(film => film.to);
if (films.length === 0) throw new Error('No film was finished in this run. Record a longer one.');
const seen = spawnSync('ffmpeg', ['-hide_banner', '-i', `${DIR}run.mp4`, '-vf', 'crop=600:50:1250:45,blackdetect=d=2:pic_th=0.97:pix_th=0.08', '-an', '-f', 'null', '-'], { encoding: 'utf8' }).stderr;
const blacks = [...seen.matchAll(/black_start:([\d.]+) black_end:([\d.]+)/g)].map(found => ({ from: Number(found[1]), to: Number(found[2]) }));
// A film is matched to the stretch of black that lasts as long as it did. Any film will do, and the best fit is taken:
// a film that ends in a dark corner of a set runs on into the card after it, and its stretch comes out too long.
const match = films.flatMap((film, number) => blacks.map(black => ({ film, number, black, off: Math.abs(black.to - black.from - (film.to - film.from)) })))
  .sort((a, b) => a.off - b.off)[0];
if (!match || match.off > 0.6) throw new Error(`No film's black bars were found in the picture (films of ${films.map(film => (film.to - film.from).toFixed(1)).join(', ')} s)`);
const pictureStarted = match.film.from - match.black.from;
console.log(`picture in step with the log: film ${match.number + 1} is ${(match.film.to - match.film.from).toFixed(2)} s in the log, ${(match.black.to - match.black.from).toFixed(2)} s in the picture`);

// The sound file does not keep the log's time. It starts a little after the log says it did, and it loses a little
// more whenever a level loads and the game stops making sound for a moment. So it is measured where it can be: a
// take starts with a clap out of silence, and the log says when that clap was asked for.
const wav = openSync(`${DIR}run.wav`, 'r');
const head = Buffer.alloc(4096);
readSync(wav, head, 0, 4096, 0);
const channels = head.readUInt16LE(22), rate = head.readUInt32LE(24), wavData = head.indexOf('data', 12, 'latin1') + 8;
const loudest = (from, seconds) => {
  const chunk = Buffer.alloc(Math.max(Math.floor(seconds * rate), 0) * channels * 2);
  readSync(wav, chunk, 0, chunk.length, wavData + Math.max(Math.floor(from * rate), 0) * channels * 2);
  let peak = 0, firstLoud = -1;
  for (let sample = 0; sample < chunk.length / 2; sample += channels) {
    const level = Math.abs(chunk.readInt16LE(sample * 2)) / 32768;
    if (level > 0.05 && firstLoud < 0) firstLoud = sample / channels / rate;
    peak = Math.max(peak, level);
  }
  return { peak, firstLoud };
};
const ahead = all('OnTakeStarted').map(take => {
  const asked = take.at - soundStarted.at;
  const found = loudest(asked - 1.5, 2).firstLoud;
  const heard = asked - 1.5 + found;
  // Only a clap that comes out of silence counts: after a "Cut!" the director is still talking when the next take starts.
  return found >= 0 && loudest(heard - 0.45, 0.4).peak < 0.02 ? { at: take.at, by: asked - heard } : null;
}).filter(Boolean);
if (ahead.length === 0) throw new Error('No take in this run starts with a clap out of silence, so the sound cannot be put in step.');
console.log(`the sound file runs ahead of the log by ${ahead.map(point => `${Math.round(point.by * 1000)} ms`).join(', then ')}`);
const soundAt = moment => moment - soundStarted.at - ([...ahead].reverse().find(point => point.at <= moment + 1) ?? ahead[0]).by;

// The run, scene by scene: where each fight was won, and what landed in it. One playthrough is enough.
const scenes = all('OnCard: "Scene ').map((card, index, cards) => ({ from: card.at, to: (cards[index + 1] ?? { at: Infinity }).at })).slice(0, HIGHLIGHTS.length);
const clips = [];
const add = (from, seconds, what) => clips.push({ from, seconds, what });

// The recording starts a few seconds into the first title card. The game shows the card again after its ending.
const title = all('OnCard: "STICK TO THE SCRIPT"').find(card => card.at + 0.6 > pictureStarted);
if (title) add(title.at + 0.6, 2.6, 'title');
const wardrobe = first('OnWardrobe: Colour');
if (wardrobe) add(wardrobe.at + 0.4, 1.5, 'wardrobe');

scenes.forEach((scene, number) => {
  const inScene = line => line.at >= scene.from && line.at < scene.to;
  const won = log.find(line => inScene(line) && line.text.includes('OnTakeFinished'));
  if (!won) return;
  const action = [...log].reverse().find(line => line.at < won.at && line.text.includes('OnAction'));
  const downs = log.filter(line => line.at > action.at && line.at <= won.at && line.text.includes('OnFighterDown: StuntPartner'));
  const blows = log.filter(line => line.at > action.at && line.at <= won.at && line.text.includes('OnHit: StickFighter'))
    .map(line => {
      const move = line.text.slice(line.text.lastIndexOf('::') + 2).trim();
      return { at: line.at, move, worth: (WORTH[move] ?? 0) + (downs.some(down => Math.abs(down.at - line.at) < 0.05) ? KNOCKOUT : 0) };
    }).sort((a, b) => b.worth - a.worth);
  const picked = [];
  for (const blow of blows) {
    if (picked.length < HIGHLIGHTS[number] && picked.every(other => Math.abs(other.at - blow.at) > 4)) picked.push(blow);
  }
  picked.sort((a, b) => a.at - b.at).forEach(blow => add(blow.at - 1.7, 3.2, `scene ${number + 1}: ${blow.move}`));

  const film = films.find(each => each.from > won.at && each.from < scene.to);
  if (number === 0) {
    add(won.at - 0.3, 1.7, 'wrap');
    const choice = log.find(line => line.at > won.at && line.text.includes('OnCard: "DIRECTOR\'S CUT"'));
    if (choice) add(choice.at + 0.5, 2.0, 'choice of cut');
  }
  if (film) add(film.from + (number === 0 ? 0.8 : 1.5), Math.min(number === 0 ? 5.7 : 4, film.to - film.from - 2), `film ${number + 1}`);
});
const picture = first('OnCard: "THAT\'S THE PICTURE"');
if (picture) add(picture.at + 0.5, 2.5, 'the end');

const length = () => clips.reduce((sum, clip) => sum + clip.seconds, 0);
// Too long: the least of the blows goes first.
while (length() > LONGEST) {
  const blows = clips.filter(clip => clip.what.startsWith('scene'));
  if (blows.length === 0) break;
  clips.splice(clips.indexOf(blows[blows.length - 1]), 1);
}
const videoLength = Number(execFileSync('ffprobe', ['-v', 'error', '-show_entries', 'format=duration', '-of', 'csv=p=0', `${DIR}run.mp4`], { encoding: 'utf8' }));
const usable = clips.filter(clip => clip.from - pictureStarted > 0 && clip.from - pictureStarted + clip.seconds < videoLength);
console.log(usable.map(clip => `${clip.what} ${clip.seconds.toFixed(1)}s`).join(' | '));
console.log(`${usable.length} pieces, ${usable.reduce((sum, clip) => sum + clip.seconds, 0).toFixed(1)} s`);

// Each piece is read twice, once from the picture and once from the sound, each from its own start.
// A tool on the developer's PC draws a frame counter along the top of every game: that strip is cut off.
const inputs = [], graph = [];
usable.forEach((clip, index) => {
  inputs.push('-ss', (clip.from - pictureStarted).toFixed(3), '-t', clip.seconds.toFixed(3), '-i', `${DIR}run.mp4`);
  inputs.push('-ss', soundAt(clip.from).toFixed(3), '-t', clip.seconds.toFixed(3), '-i', `${DIR}run.wav`);
  graph.push(`[${index * 2}:v]crop=1877:1056:21:24,scale=1920:1080,fps=60,setsar=1[v${index}]`);
  graph.push(`[${index * 2 + 1}:a]afade=t=in:d=0.04,afade=t=out:st=${(clip.seconds - 0.06).toFixed(3)}:d=0.06,aresample=48000[a${index}]`);
});
graph.push(`${usable.map((clip, index) => `[v${index}][a${index}]`).join('')}concat=n=${usable.length}:v=1:a=1[v][a]`);
execFileSync('ffmpeg', ['-v', 'error', '-y', ...inputs, '-filter_complex', graph.join(';'), '-map', '[v]', '-map', '[a]',
  '-c:v', 'libx264', '-preset', 'slow', '-crf', '19', '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-b:a', '192k', '-movflags', '+faststart', OUT], { stdio: 'inherit' });

// For whoever checks it without watching it: one frame every two seconds on a sheet, and how loud the sound is.
execFileSync('ffmpeg', ['-v', 'error', '-y', '-i', OUT, '-vf', 'fps=0.5,scale=384:-1,tile=6x5', '-frames:v', '1', `${DIR}sheet.jpg`]);
const loud = spawnSync('ffmpeg', ['-hide_banner', '-i', OUT, '-af', 'volumedetect', '-vn', '-f', 'null', '-'], { encoding: 'utf8' }).stderr;
console.log((loud.match(/mean_volume: [-\d.]+ dB/) ?? ['no sound measured'])[0], '|', (loud.match(/max_volume: [-\d.]+ dB/) ?? [''])[0]);
console.log(`written: ${OUT}`);
