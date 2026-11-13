---
sprint: 1
stream: feel
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-CORE-01 Shared types and events]]"
---

**Board tidy, Fri Nov 13:** in the game since the first day; the developer has played it.

**Goal:** The player always knows the beat, the time left and the take number, and gets a "Cut!" slate with the reason.

**You own:**
- `Source/StickToTheScript/UI/`
- `Content/UI/`

**Done when:**
- [x] Script card, beat cue with timer, REC overlay with take number, Cut! slate (the script card is only a take card so far)
- [x] Driven only by the events in [[Contracts]]

**Notes:** Never read the director directly. Listen to events, so the interface cannot break the take loop.

The beat cue is a prompt drawn on the fighter the beat is about: one circle per move that counts, with its key inside and a ring that closes as the time runs out (asked for by the developer in place of the first version's bar). The script line stays as a subtitle.

Built as `AStickHUD`, drawn on the canvas with the engine font, so there is no interface asset yet. Still open for S2-FEEL-02: the font is a scaled bitmap and looks soft, the keys in the prompts are written by hand, and the card before a take shows the take number but not the scene's beats.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: feel
