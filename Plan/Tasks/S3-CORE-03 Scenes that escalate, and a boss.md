---
sprint: 3
stream: core
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-CORE-02 Fists before guns]]"
---

**Goal:** The three scenes are no longer the same fight three times. The warehouse is the lightest, the bar is a crowd in a small room, and the rooftop ends on a boss.

**Why:** in the recorded run of Sat Nov 14 the game's own player won every scene in its first take, in about 40 seconds each, taking 6, 6 and 7 blows. Every scene had six opponents. In the editor the bar was the hard one (2 to 5 takes) and the other two never cost a retake.

**You own:**
- `Content/Scripts/Scene*.json`
- `Source/StickToTheScript/Core/TakeTypes.h` (one more kind of opponent, appended)
- `Source/StickToTheScript/Fighter/StuntPartner.*`

**What was done:**
- The warehouse has five opponents instead of six: one rusher fewer.
- The bar keeps six, with a kicker where its second rusher was: rushers are what swarm.
- The rooftop keeps its six and gets a seventh, the boss: a heavy who does not plod (300 health, armoured, guards more, rests less, half of what he throws is a big move). He stands at the helipad by the kid and comes on when everybody else is down, or when he is hit.
- As the boss comes on the hero is put back to full health ("Make-up!"), so the last fight starts even: there is no other way to get health back in a take.
- The boss has a bar of his own along the top of the screen.

**Done when:**
- [x] The scene files load with the new kind (`Tools/check.ps1`)
- [x] The boss comes on, has his bar, and the hero is put right: seen in the editor
- [x] How many takes the game's own player needs for each scene, in the editor on Sat Nov 14: the warehouse 1 and 1; the bar 4, 1 and 2; the rooftop 1, 1 and 1 with the boss as first written, which made him no peak. With 300 health, harder blows and more guarding: 2, 1, 2 and 2
- [x] The packaged build has it: run off screen from start to finish on Sat Nov 14 at 20:10, it won the warehouse in one take, the bar in four and the rooftop with its boss in one, and reached the closing card with no error
- [ ] The developer has played it and says whether the boss is fair

Sprint: [[Sprint 3 - Feel and lock]] · Stream: core
