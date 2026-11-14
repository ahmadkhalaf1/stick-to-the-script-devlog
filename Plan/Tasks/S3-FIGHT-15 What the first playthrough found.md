---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 1
depends:
  - "[[S3-SHIP-02 Bug bash and feature freeze]]"
---

**Goal:** The three things the developer reported while playing the evening's build on Sat Nov 14, the first time a person played it that day.

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*`, `Fighter/ThrownProp.cpp`
- `Source/StickToTheScript/UI/StickHUD.cpp`, `Core/GameFlow.cpp` (the buttons)

**What was reported, and what was done:**
- "holding baseball bat how can i still punsh? hit should hit with the bat". With a bat in the hands the click played the plain punch and the bat rode along upright, so it looked like a fist, though the blow counted as the bat's. The bat now comes over and down as the blow arrives, with both hands on it, and goes back up.
- "at menu should be continue or next butotn or start not only clickign mouse , it snot obvious". Enter and Space already went on everywhere, and nothing on the screen said so. The chosen scene card now wears a gold PLAY button, the wardrobe has a "Start the scene" button, the closing card and the credits have a "Back" button, every menu's hint says "Enter or click", and the film says "Enter or click to skip".
- "crouch + walking is not working i need to release crouch key first". A held duck stopped all walking. Ducked, the fighter now creeps at half its walking speed. Behind a guard it still stands its ground.

**Done when:**
- [x] The bat's swing seen in the editor, photographed in slow motion
- [x] The PLAY button and the wardrobe's button seen in the editor
- [ ] Creeping tried: it needs a key held down, which the editor's tools cannot do
- [ ] The packaged build has all three
- [ ] The developer has tried them

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
