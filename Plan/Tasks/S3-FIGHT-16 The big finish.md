---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 3
depends:
  - "[[S3-FIGHT-15 What the first playthrough found]]"
---

**Goal:** What the developer asked for on Sat Nov 14, in their words: "a power bar that get filled by hitting enemies, then provide a cinematic finish move by showing an overlay button icon asking to finish the enemies and it should show a nice animation with High quality cinematic view and object destruction where enemies get destroyed into parts".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*` (the move)
- `Source/StickToTheScript/Core/StickPlayerController.*` (the bar, the key, the camera), `Core/TakeEvents.*` (one event)
- `Source/StickToTheScript/UI/StickHUD.cpp`, `UI/ImpactEffects.*`

**What it is, built from what the game already had:**
- A gold bar under the hero's health. Every blow that lands fills it, a big blow more: about a dozen fill it. A new take empties it.
- Full, it puts a key on the screen: Q, and "FINISH THEM".
- Q starts three blows on the nearest opponent within 3.4 m, with clips the game already had: a hook that stops him, an uppercut that lifts him off the floor, and a spinning kick. Nothing can touch or interrupt the hero until the last has landed; a clock gives it up after three seconds if something stops it. (The first version was the kick alone. The developer asked for "more than 1 move".)
- The kick reaches everyone within 3.4 m, all round. Whoever it reaches is put down and loses both forearms and both shins, with the limbs guns already took off. Anyone with more than 200 health, which is the boss, is hurt badly instead (110 of his 300).
- Each blow has its own picture, from the film's two close cameras: over the hero's shoulder, from the floor looking up, and from further back for the kick. Time runs at two fifths throughout. 2.4 seconds after the last blow starts, the picture goes back to where it was.
- The game's own player spends a full bar after it has been on the screen for 1.2 seconds, so the video shows the key.

**Done when:**
- [x] The bar fills and the kick puts down everyone near, in pieces: seen in the editor, 16 limbs in one fight
- [x] The cameras take the picture, blow by blow, and give it back: photographed every 0.4 seconds
- [x] The key on the screen, seen
- [x] The bodies stay in the close picture (the first try knocked them out of it)
- [x] The film of a take with a finish in it plays to its slate, no error
- [ ] The finish tried on the boss
- [x] The packaged build has it: five of them in one self-played run on Sat Nov 14, 22:14
- [ ] The developer has pressed Q

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
