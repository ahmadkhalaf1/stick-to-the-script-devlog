---
sprint: 1
stream: core
status: done
owner: lead
estimate: 5
depends:
  - "[[S1-CORE-01 Shared types and events]]"
---

**Goal:** Runs a scene script beat by beat, judges the player's move, and either advances or calls "Cut!".

**You own:**
- `Source/StickToTheScript/Core/TakeDirector.*`

**Done when:**
- [x] A hard-coded four-beat script can be played with debug keys
- [x] A wrong move, an off-cue attack or a timeout fires OnCut with the right reason and restarts with the take number raised by one
- [x] Four correct moves fire OnTakeFinished
- [x] An automation test covers the judging rule

**Notes:** Judging is a pure function of beat and move: keep it testable without a level.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: core
