---
sprint: 2
stream: cam
status: done
owner: lead
estimate: 3
depends:
  - "[[S1-CAM-02 Take recorder]]"
---

**Board tidy, Fri Nov 13:** never built. Failed takes are not kept.

**Built on Sat Nov 14**, asked for by the developer: "Yes, blooper reel".

**Goal:** Failed takes are kept and the best ones play after the film.

It was planned as one reel under the credits. It became the outtakes of each scene, shown straight after that scene's film: a take can only be played back on the set it was shot on, and each scene is its own level.

**How it works:**
- The recorder (`Cam/FightRecorder`) used to stop at "Cut!". It now records 1.4 seconds more, so the fall is in it, and keeps the last 3.4 seconds of that take as an outtake. It keeps the latest three.
- The film (`Cam/FinalCut`), after its slate, plays each outtake with the machinery it plays the film with: the same puppets and props, the same three cameras with close-ups on the blows, the blood and the sounds. At its own speed, with no slow motion.
- `OnOuttake(TakeNumber, bCut)` says when one starts and when its "Cut!" comes. The sound claps it in and plays the director's "Cut!". The HUD writes "OUTTAKES   TAKE n" in the top bar and, from the cut, "CUT!" with "Exactly as the script said."
- A punch skips the film and the outtakes together. A scene wrapped on its first take has none.

**You own:**
- `Source/StickToTheScript/Cam/`

**Done when:**
- [x] The end of a take that was cut is kept, fall included, and only the latest three: by an automation test
- [x] After the film the outtakes play, each with its take number and its "Cut!": seen in the editor in the bar, two takes lost standing still and the third won by the game playing itself
- [ ] A reel of three failed takes has been seen (two were)
- [ ] The packaged build shows them. It has the code, and five minutes of it playing itself showed four films and no errors, but its player was never knocked out, so no outtake came up
- [ ] The developer has seen it

Sprint: [[Sprint 2 - Three scenes]] · Stream: cam
