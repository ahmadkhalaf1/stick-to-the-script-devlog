---
sprint: 2
stream: core
status: done
owner: lead
estimate: 3
depends:
  - "[[S2-FIGHT-04 Free fight]]"
---

**Goal:** The pay scores the film, not only survival. A fight that shows the audience many different things earns more than one that wins by repeating a single move. This is what makes the free fight a "Director's Cut": the player fights for the camera.

**You own:**
- `Source/StickToTheScript/Core/GameFlow.*` (the score and the cards)
- `Source/StickToTheScript/UI/StickHUD.*` (the live figure)

**Done when:**
- [x] Every blow the player lands in the shoot earns money: a punch $2, a kick $5, a thrown prop $30, anything from the air $20, a knockout $20 on top, a blow stopped on the guard $10
- [x] The same kind of move pays three times running, then nothing until another kind is used ("seen it before"); a knockout always pays
- [x] At the end, every kind of move beyond the first adds $40
- [x] The pay at the top right climbs as it is earned, and the last thing earned shows under it
- [x] The pay card after the film lists the fee, the retakes, the box office and what it was for
- [x] The results card at the end of the run splits fees from box office
- [x] The film does not pay again for the blows it replays
- [x] A test covers the rules (`BoxOfficePaysForVariety`)

**Notes:** Proposed by the agent when the developer asked what would make the free fight fit the theme, and approved by the developer ("Yes, box office") on Thu Nov 12.

The amounts are first guesses, in one block at the top of `GameFlow.cpp`. On self-play the warehouse paid $334 at the box office on top of a $500 fee. A person who jumps, guards and throws should beat that; whether the balance feels right is for the developer to say.

Only the take that is the one counts: the box office starts from nothing at every retake.

Sprint: [[Sprint 2 - Three scenes]] · Stream: core
