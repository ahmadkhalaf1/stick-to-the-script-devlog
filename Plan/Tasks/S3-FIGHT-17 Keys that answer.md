---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-16 The big finish]]"
---

**Goal:** What the developer said on Sat Nov 14, after playing: "i still find controls and combining keys while playing hard and sticky laggy an dnot responsive enough need to be checked and improved".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*` (what a fighter lets a key do, and when)
- `Source/StickToTheScript/Core/StickPlayerController.*` (the keys)
- `Source/StickToTheScript/UI/ImpactEffects.cpp` (the freeze on a blow)

**What was checked first:** the frame rate, from the log of the developer's own session. About 95 frames a second with the PC quiet, and about 90 in the two and a half minutes the developer was fighting. It fell to between 30 and 60 while the agent's editor tests ran beside the game, but the game was standing unplayed then. So the frame rate was not it; the game's own handling of the keys was.

**What was wrong, and what was done:**
- A held key only counted once the body had got into its pose. Duck and punch pressed together gave a plain punch about half the time, and always did while another strike was still playing out. Now the key itself counts, at the moment of the press; and when the strike key lands first, a held key that follows within 0.09 s turns the strike that has just begun into the move the two mean.
- The guard and the duck were ignored until a strike's whole follow-through was over: 0.71 s from pressing kick. Now they cut it short 0.04 s after a punch lands and 0.12 s after anything else. Walking off and jumping do the same. The next strike comes no sooner for it.
- Up to three key presses were kept, each for 0.75 s, so the fighter went on hitting for most of a second after the hands had stopped. Now one is kept, the newest.
- Every blow froze the game: 0.09 s, 0.14 s for a heavy one, 0.04 s for a block. Now 0.05, 0.09 and 0.02.
- The player's strikes arrive sooner: punch 0.13 s (was 0.16), kick 0.22 (0.26), uppercut 0.17 (0.2), sweep 0.2 (0.24), hook 0.24 (0.28), roundhouse 0.3 (0.36). The next kick may follow 0.22 s after one lands (0.3).
- Behind a raised guard the player's fighter walks, slowly. It used to be rooted.
- A blow holds the player's fighter helpless for three quarters as long as before.
- Opponents are as they were, except that they are frozen for less on a blow like everyone.

**Done when:**
- [x] Duck and punch give the uppercut whichever key lands first, and in the middle of another strike: timed in the editor, the punch turned 0.010 s after it began
- [x] The guard comes up out of a strike that has landed: timed, 0.12 to 0.13 s after a kick or any of the four others lands
- [x] The rhythm of repeated punches and kicks, timed (a punch every 0.25 to 0.28 s, a kick every 0.44), and one strike at most comes after the keys stop
- [x] The game still plays itself through all three scenes: one take, one, two in the editor
- [x] The packaged build has it: made on Sat Nov 14 at 22:13 and played by itself from the first menu to the end card
- [ ] The developer has played it and says whether it answers now
- [ ] Walking out of a strike and walking behind a guard, tried with a held key (the editor tests can only tap)

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
