---
sprint: 3
stream: feel
status: done
owner: lead
estimate: 1
depends:
  - "[[S3-FIGHT-07 Real fight animations]]"
---

**Goal:** The game is played with the keys a PC player reaches for without being told, and it says which they are.

Asked for by the developer on Fri Nov 13, on playing the new animations: "keybinds are hard and not usual", then "maybe right mouse should be used to block hits".

**You own:**
- `Source/StickToTheScript/Core/StickPlayerController.cpp` (the bindings)
- `Source/StickToTheScript/UI/StickHUD.*`, `Source/StickToTheScript/Core/GameFlow.cpp` (what the game says the keys are)

**The keys now:** W A S D move, left click punch, right click guard (held), F kick, Ctrl duck (held), Space jump, E grab and throw, Esc pause. The old ones still work: J, K, L, C, P, Shift for the guard, the arrow keys, and the gamepad as before. Two things moved: the right mouse button was the kick and is the guard, and F was a second key for grabbing and is the kick.

**Done when:**
- [x] Left click punches and F kicks: three clicks and one F in a take in the editor threw punches and a kick, by the log
- [x] Holding the right mouse button guards. It is read the same way as Shift, which guards. The agent could not try it; the developer played with the keys and found them good, without naming the guard
- [x] The game names these keys, not the old ones: on the line shown as a take is called, on the pause card, in the wardrobe and under the menus
- [x] The keys stay on screen for the first 14 seconds of a scene's first take, unless the director is talking over them. They used to show only while "and action!" did
- [x] The store page's table of controls says the same (`Docs/StorePage.md`; the draft page itself is brought up to date with the final build)
- [x] The developer has played with them (Fri Nov 13, 17:45): "keys good"

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
