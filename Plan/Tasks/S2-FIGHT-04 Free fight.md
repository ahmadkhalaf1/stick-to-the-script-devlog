---
sprint: 2
stream: fight
status: done
owner: lead
estimate: 10
depends:
  - "[[S2-FIGHT-03 Fights that flow]]"
---

**Board tidy, Fri Nov 13:** the game is this.

**Goal:** Nobody tells the player what to press. The script says the hero loses; the player walks in and fights the scene their own way, and the film is cut from whatever they did.

**You own:**
- `Source/StickToTheScript/Fighter/` (free movement, strikes, health, opponents that think, props)
- `Source/StickToTheScript/Core/` (controls, the director as referee, scene format)
- `Source/StickToTheScript/UI/`, `Cam/` (health, the director's shouts, props in the film)
- `Content/Scripts/`, `Tools/build_stage01.py`

**Done when:**
- [x] No prompts, circles or cue lines: a scene with no beats is a free fight
- [x] The player walks in two directions, punches and kicks with keys of their own, and a strike turns to the nearest opponent in reach
- [x] A blow lands only when the fist or foot reaches the body. Measured: the limb stops a median of 8 to 17 cm from the body's centre line
- [x] The kick is a kick, posed in code, not the charged punch that stood in for it
- [x] Four kinds of opponent that think for themselves: at most two come at the player while the rest keep their distance, take sides and wait; some raise a guard; a hit costs an opponent its turn
- [x] All opponents down is a wrap and the film plays; the player down is "Cut!" and a retake
- [x] Crates and bottles can be picked up and thrown, and a thrown crate hurts whoever it reaches
- [x] The script card says what was written, and the director shouts when the player ignores it
- [ ] Played by a person: walking, ducking, guarding and jumping need held keys, which the agent's tools cannot hold
- [ ] The film shows the props flying (recorded, never looked at)
- [x] A packaged build with all of this in it (632 MB; it plays both scenes on self-play)

**Found by packaging and by the developer watching:** a clean build did not compile (two helpers shared a name across files), and a fighter reset after a knockout was drawn a metre away from where it stood. Both fixed; `Tools/check.ps1` now builds the way a clean checkout does, and the game logs a warning if a fighter is ever above the floor without a jump.

**Notes:** Asked for by the developer on Thu Nov 12: "more of a free fight like street fights, no more button animation or view circle, u roam around and hit everything in your way ... we call it as u dont stick to script and u rebel".

Keys: W A S D or arrows to move, J or left mouse to punch, K or right mouse to kick, L or Shift to guard, C or Ctrl to duck, Space to jump, E to pick up and throw. A punch or kick with something in the hands throws it.

Opponents (`EOpponentKind` in a scene file): Thug (balanced), Rusher (fast, weak, hits and runs), Kicker (kicks, backs off), Heavy (slow, strong, punches do not stop him).

The beat machinery (`FBeat`, the prompts, `JudgeMove`) was deleted on Fri Nov 13: 821 lines out of 23 files. The developer said to, before having played the free fight by hand.

Not built, still wanted by the developer: drags, climbing, and "cinematic crazy moves". Opponents do not pick things up.

Sprint: [[Sprint 2 - Three scenes]] · Stream: fight
