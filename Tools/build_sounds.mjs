// Makes the game's recorded sounds: blows, the director's voice and the music, asked for in words from fal.ai
// (ElevenLabs sound effects, speech and music). The list below is the source: what was asked for is what is written here.
//
//   node Tools/build_sounds.mjs            says what is missing and what it would cost. Asks for nothing.
//   node Tools/build_sounds.mjs --go       asks for what is missing
//   node Tools/build_sounds.mjs --go Punch only the names that start like this
//   node Tools/build_sounds.mjs --hear     has every spoken line written down again by a machine, to compare with the script
//
// It writes WAV files to Saved/Audio, and keeps what fal sent in Saved/Audio/raw. Tools/build_audio.py then brings the
// WAV files into Content/Audio. A file that is already there is left alone. To make one again, delete it. To use a
// recording of your own, put it there under the same name and this script will not touch it.
//
// The key is read from the FAL_KEY environment variable and never printed. ffmpeg has to be installed.

import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const OUT = fileURLToPath(new URL('../Saved/Audio/', import.meta.url));
const SCENES = fileURLToPath(new URL('../Content/Scripts/', import.meta.url));
const RATE = 44100;

// Dollars, as fal listed them on Fri Nov 13 2026.
const EFFECT_PER_SECOND = 0.002;
const SPEECH_PER_CHARACTER = 0.1 / 1000;
const MUSIC_PER_MINUTE = 0.6;

const DIRECTOR = 'Callum';
const FOLEY = 'film foley, close, dry, no reverb, no music';

// S_<name>_<take> is a sound the game otherwise makes in code; the name is one of EImpactSound, or Action, Cut and Wrap.
// V_Scene<n>_<line> is a line the director shouts in scene n. M_Fight_<n> is that scene's fight music, M_Film the film's.
// fal's filter refuses a request that describes someone being hurt, so the heavy blow is asked for as a sandbag and a bat.
const effects = [
  ['Punch', 3, 0.8, `One hard punch landing, a tight meaty thud like a fist on a heavy bag, martial arts ${FOLEY}`],
  ['HeavyHit', 3, 0.8, `A heavy sandbag struck hard with a wooden bat, one deep thud with a sharp smack on top, ${FOLEY}`],
  ['BoneBreak', 2, 0.6, 'A single dry snap, one sharp crack like a thick stick of celery breaking, close, no reverb, no music'],
  ['Knockout', 2, 1.8, 'A huge final knockout blow in an action film, a heavy body impact and then the body dropping on a hard floor, no music'],
  ['Block', 3, 0.8, `A punch stopped by a raised forearm, a dull slap of arm on arm, martial arts ${FOLEY}`],
  ['Whoosh', 3, 0.5, 'A fist swung fast through the air, one short quick whoosh, dry, no music'],
  ['SpinWhoosh', 2, 0.7, 'A spinning kick cutting through the air, one longer rising whoosh, dry, no music'],
  ['Shout', 3, 0.6, 'A male martial artist shouting one short sharp "hah!" as he strikes, kung fu film, dry'],
  ['Clang', 2, 0.9, 'Two hands grabbing a hollow steel scaffolding pipe, a short metallic clang with a brief ring, no music'],
  ['Clapper', 1, 0.5, 'A film clapperboard snapped shut once, a sharp wooden clack, close, dry'],
  ['Gunshot', 2, 1.0, 'A single pistol shot indoors, a loud sharp crack with a short tail, no music'],
];
// A direction in square brackets is acted, not read out. The two shortest lines have none: with one, the direction took
// longer than the line, and nothing here can hear which of the two was said.
const calls = [
  ['S_Action_1', 'And... ACTION!'],
  ['S_Action_2', '[shouting] Action!'],
  ['S_Action_3', '[shouting] Quiet on the set! And... action!'],
  ['S_Cut_1', '[shouting, furious] CUT! Cut, cut, cut!'],
  ['S_Cut_2', 'CUT!'],
  ['S_Cut_3', '[tired, shouting] Cut. Back to one, everybody.'],
  ['S_Cut_4', '[shouting] Cut! Somebody help him up. We go again.'],
  ['S_Cut_5', '[shouting, pleased] Cut! Now THAT fall was in the script.'],
  ['S_Wrap_1', "[shouting, surprised] That's... a wrap?"],
  ['S_Wrap_2', "[sighs] Fine. That's a wrap."],
  ['S_Wrap_3', "[defeated] Wrap. That's a wrap. Somebody call my agent."],
];
// What the director shouts during a fight is written in each scene's own file, and read from there: the game shows
// those words and this asks for the same words spoken. A line that starts with dots is said under his breath.
const shouts = readdirSync(SCENES).filter(name => name.startsWith('Scene') && name.endsWith('.json')).sort()
  .flatMap((name, scene) => (JSON.parse(readFileSync(SCENES + name, 'utf8')).DirectorLines ?? [])
    .map((line, index) => [`V_Scene${scene + 1}_${index + 1}`, `${line.startsWith('...') ? '[quietly, to the crew]' : '[shouting]'} ${line}`]));
const lines = [...calls, ...shouts];

const FIGHT = '1970s kung fu film fight music, instrumental, steady all the way through so it can loop';
const tracks = [
  ['M_Fight_1', 60, `Tense, funky ${FIGHT}: driving drums, wah guitar, a punchy bass line and short brass stabs, 120 beats a minute`],
  ['M_Fight_2', 60, `Swaggering bar-room ${FIGHT}: a honky-tonk piano riff over a funk drum groove, slap bass and handclaps, 112 beats a minute`],
  ['M_Fight_3', 60, `Urgent night-time rooftop showdown, ${FIGHT}: fast hi-hats, a tense string ostinato, deep bass and big brass hits, 132 beats a minute`],
  ['M_Film', 60, 'A fast, triumphant 1970s kung fu film theme: the full band, a bold brass melody over driving drums and bass, 140 beats a minute, instrumental'],
];

const wanted = [
  ...effects.flatMap(([name, takes, seconds, text]) => Array.from({ length: takes }, (unused, take) => ({
    file: `S_${name}_${take + 1}`, cost: seconds * EFFECT_PER_SECOND, quiet: true,
    model: 'elevenlabs/sound-effects/v3', body: { text, duration_seconds: seconds, prompt_influence: 0.4 },
  }))),
  ...lines.map(([file, text]) => ({
    file, cost: text.length * SPEECH_PER_CHARACTER, quiet: true, spoken: text,
    model: 'fal-ai/elevenlabs/tts/eleven-v3', body: { text, voice: DIRECTOR, stability: 0.3, timestamps: true },
  })),
  ...tracks.map(([file, seconds, prompt]) => ({
    file, cost: Math.ceil(seconds / 60) * MUSIC_PER_MINUTE, quiet: false, stereo: true,
    model: 'fal-ai/elevenlabs/music', body: { prompt, music_length_ms: seconds * 1000, force_instrumental: true },
  })),
];

function key() {
  if (process.env.FAL_KEY) return process.env.FAL_KEY.trim();
  // A variable set with setx is not seen by programs that were already running, so it is read from where Windows keeps it.
  try {
    return execFileSync('powershell', ['-NoProfile', '-Command', "[Environment]::GetEnvironmentVariable('FAL_KEY', 'User')"], { encoding: 'utf8' }).trim();
  } catch {
    return '';
  }
}

async function call(model, body, secret) {
  const answer = await fetch(`https://fal.run/${model}`, {
    method: 'POST',
    headers: { Authorization: `Key ${secret}`, 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
  if (!answer.ok) {
    throw new Error(`fal said ${answer.status}: ${(await answer.text()).slice(0, 220)}`);
  }
  return answer.json();
}

async function ask(sound, secret) {
  const made = await call(sound.model, sound.body, secret);
  if (!made.audio?.url) {
    throw new Error(`fal sent no audio: ${JSON.stringify(made).slice(0, 220)}`);
  }
  // The timing of each character shows whether a direction was acted or read out: acted, "[shouting]" passes in a
  // fifth of a second; read out, it takes as long as the word does.
  if (sound.spoken && Array.isArray(made.timestamps)) {
    const characters = made.timestamps.flatMap(part => part.characters ?? []);
    const starts = made.timestamps.flatMap(part => part.character_start_times_seconds ?? []);
    const ends = made.timestamps.flatMap(part => part.character_end_times_seconds ?? []);
    const open = characters.indexOf('['), close = characters.indexOf(']');
    if (open >= 0 && close > open && ends[close] - starts[open] > 0.03 * (close - open) + 0.25) {
      console.warn(`${sound.file}: the stage direction took ${(ends[close] - starts[open]).toFixed(2)} s. Check it with --hear.`);
    }
  }
  return Buffer.from(await (await fetch(made.audio.url)).arrayBuffer());
}

// What comes back is an MP3 with air at both ends, at whatever level it happened to come out. The game wants a WAV that
// starts on the sound, ends when it has died away, and is as loud as the others. Quiet is judged against the clip's own
// loudest moment: judged against full scale, a soft clip was cut away to nothing.
function finish(sound, mp3) {
  mkdirSync(`${OUT}raw`, { recursive: true });
  const raw = `${OUT}raw/${sound.file}.mp3`, out = `${OUT}${sound.file}.wav`;
  writeFileSync(raw, mp3);
  const channels = sound.stereo ? 2 : 1;
  execFileSync('ffmpeg', ['-v', 'error', '-y', '-i', raw, '-ac', String(channels), '-ar', String(RATE), '-c:a', 'pcm_s16le', out]);
  const wav = readFileSync(out);
  rmSync(out);
  const data = wav.indexOf('data', 12, 'latin1') + 8;
  const frames = Math.floor((wav.length - data) / 2 / channels);
  const at = (frame, channel) => wav.readInt16LE(data + (frame * channels + channel) * 2);
  const level = frame => Math.max(Math.abs(at(frame, 0)), Math.abs(at(frame, channels - 1)));

  let peak = 0;
  for (let frame = 0; frame < frames; frame++) peak = Math.max(peak, level(frame));
  if (peak < 300) {
    throw new Error('came back silent');
  }
  let first = 0, last = frames - 1;
  if (sound.quiet) {
    while (level(first) < peak * 0.03) first++;
    while (level(last) < peak * 0.006) last--;
    first = Math.max(first - Math.floor(RATE * 0.002), 0);
    last = Math.min(last + Math.floor(RATE * 0.02), frames - 1);
  }

  const kept = last - first + 1;
  const fadeIn = Math.floor(RATE * 0.002);
  const fadeOut = sound.quiet ? Math.min(Math.floor(RATE * 0.03), kept) : 1;
  const gain = (sound.stereo ? 0.7 : 0.9) * 32767 / peak;
  const body = Buffer.alloc(kept * channels * 2);
  for (let frame = 0; frame < kept; frame++) {
    const shape = Math.min(1, (frame + 1) / fadeIn, (kept - frame) / fadeOut);
    for (let channel = 0; channel < channels; channel++) {
      body.writeInt16LE(Math.round(at(first + frame, channel) * gain * shape), (frame * channels + channel) * 2);
    }
  }
  const header = Buffer.alloc(44);
  header.write('RIFF', 0, 'latin1');
  header.writeUInt32LE(36 + body.length, 4);
  header.write('WAVEfmt ', 8, 'latin1');
  header.writeUInt32LE(16, 16);
  header.writeUInt16LE(1, 20);
  header.writeUInt16LE(channels, 22);
  header.writeUInt32LE(RATE, 24);
  header.writeUInt32LE(RATE * channels * 2, 28);
  header.writeUInt16LE(channels * 2, 32);
  header.writeUInt16LE(16, 34);
  header.write('data', 36, 'latin1');
  header.writeUInt32LE(body.length, 40);
  writeFileSync(out, Buffer.concat([header, body]));
  return kept / RATE;
}

const secret = key();
const needsKey = () => {
  if (!secret) {
    console.error('No FAL_KEY. Set it with: setx FAL_KEY "<the key from fal.ai/dashboard/keys>"');
    process.exit(1);
  }
};

if (process.argv.includes('--hear')) {
  needsKey();
  for (const sound of wanted.filter(each => each.spoken && existsSync(`${OUT}${each.file}.wav`))) {
    // fal's listener takes a file written into the request only as MP3.
    const mp3 = execFileSync('ffmpeg', ['-v', 'error', '-i', `${OUT}${sound.file}.wav`, '-f', 'mp3', '-'], { maxBuffer: 1 << 26 });
    const audio = `data:audio/mpeg;base64,${mp3.toString('base64')}`;
    try {
      const heard = await call('fal-ai/whisper', { audio_url: audio, task: 'transcribe', language: 'en', chunk_level: 'none' }, secret);
      console.log(`${sound.file}\n  asked: ${sound.spoken}\n  heard: ${String(heard.text).trim()}`);
    } catch (error) {
      console.error(`${sound.file}: ${error.message}`);
    }
  }
  process.exit(0);
}

const go = process.argv.includes('--go');
const only = process.argv.slice(2).filter(word => !word.startsWith('--'));
const missing = wanted.filter(sound => !existsSync(`${OUT}${sound.file}.wav`))
  .filter(sound => only.length === 0 || only.some(start => sound.file.includes(`_${start}`) || sound.file.startsWith(start)));
const cost = missing.reduce((sum, sound) => sum + sound.cost, 0);
console.log(`${wanted.length} sounds in the list, ${missing.length} to make, about $${cost.toFixed(2)}`);
if (!go) {
  console.log(missing.map(sound => sound.file).join(' '));
  process.exit(0);
}

needsKey();
mkdirSync(OUT, { recursive: true });
let spent = 0, failed = 0;
for (const sound of missing) {
  try {
    const seconds = finish(sound, await ask(sound, secret));
    spent += sound.cost;
    console.log(`${sound.file}: ${seconds.toFixed(2)} s`);
  } catch (error) {
    failed++;
    console.error(`${sound.file}: ${error.message}`);
  }
}
console.log(`made ${missing.length - failed} of ${missing.length}, about $${spent.toFixed(2)} spent`);
process.exit(failed ? 1 : 0);
