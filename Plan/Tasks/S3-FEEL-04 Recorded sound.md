---
sprint: 3
stream: feel
status: done
owner: lead
estimate: 4
depends:
  - "[[S2-FEEL-01 Final cut sound]]"
---

**Goal:** The game sounds like a film being shot: recorded blows, a director with a voice, real music.

Asked for by the developer on Fri Nov 13: "in 2026 , and AI and claude , the game feel poor", then, about the jam's partner perks, "Yes, I'll claim them", and while it was being built: "keep in mind different text and voice over at different scenes not always the same".

**You own:**
- `Source/StickToTheScript/UI/ImpactEffects.*`, `FilmScore.*`, `StickHUD.*`
- `Tools/build_sounds.mjs`, `Tools/build_audio.py`, `Content/Audio/`
- `"DirectorLines"` in `Content/Scripts/Scene*.json`

**Done when:**
- [x] A sound with a recording in `Content/Audio` plays the recording; one without is made in code as before
- [x] Several takes of one sound, picked at random
- [x] The director calls "Action!", "Cut!" and "That's a wrap" out loud
- [x] Every scene has its own lines for the director, written and spoken, and its own fight music
- [x] The words every sound was asked for with are in the repository
- [x] Every spoken line checked against the script by a machine, since the agent cannot hear
- [x] All of it is in the packaged game
- [ ] The developer has heard it
- [ ] Loudness of music against blows against voice, set by ear
- [x] Frame rate measured again on an idle PC: median 98 in the warehouse, 107 in the bar and 103 on the rooftop, and that while the window was being recorded. The recorded sound costs nothing; the low readings before were the other program

**How it works:**
- `node Tools/build_sounds.mjs` lists what is missing and what it costs; `--go` asks fal.ai for it (needs `FAL_KEY` and ffmpeg); `--hear` has the spoken lines written down again by a speech-to-text model.
- `Tools/build_audio.py` brings the WAV files of `Saved/Audio` into `Content/Audio`. Its header says how they are named.
- To change a line: edit it in the scene's file, delete its WAV in `Saved/Audio`, run both scripts.
- To use a recording from somewhere else: put a WAV in `Saved/Audio` under the right name and run `Tools/build_audio.py`.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
