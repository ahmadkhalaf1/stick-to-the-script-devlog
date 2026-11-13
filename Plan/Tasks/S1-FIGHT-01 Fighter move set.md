---
sprint: 1
stream: fight
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-CORE-01 Shared types and events]]"
---

**Board tidy, Fri Nov 13:** in the game; more moves came in S3-FIGHT-05.

**Goal:** AStickFighter can play each move on request and reports the frame its attack lands.

**You own:**
- `Source/StickToTheScript/Fighter/ (move StickFighter.* here)`
- `Content/Characters/`

**Done when:**
- [x] PlayMove covers three attacks, the charged attack, hit reactions, knockdown and idle
- [x] OnHit fires once per attack at the contact frame
- [ ] A test level cycles through every move

**Notes:** Uses the engine animations already in Content/Characters/Mannequins.

- The three punches cycle on Punch. Kick plays the charged punch until S2-FIGHT-01 brings a real kick.
- The engine hit reactions are additive and cannot play alone, so a hit is a flinch computed on the joints. The engine death animations end standing, so a knockout is a ragdoll.
- Contact is found by stepping through each strike and taking the moment a hand or foot reaches furthest forward (0.40 s for the first punch, 1.13 s for the charged one).
- Still open: `bDemoAllMoves` exists for the test level, but nobody has watched it and no test level is saved.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: fight
