---
sprint: 2
stream: feel
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-CAM-02 Take recorder]]"
---

**Board tidy, Fri Nov 13:** wardrobe, hats, builds and colours are in.

**Goal:** The player picks a simple look for their fighter, and the Director's Cut shows that exact character, so the film feels like theirs.

**You own:**
- `Source/StickToTheScript/Fighter/` (how a look is drawn)
- `Source/StickToTheScript/UI/` (how it is picked)

**Built Thu Nov 12:** a wardrobe after the title card. A / D changes the colour (eight), W / S the hat (none, cap, top hat, headband, mohawk), K the build (slim, regular, heavy); punch starts the scene. The look is kept for every scene and, because the film poses the same fighter, it is in every film. Not saved between sessions. Seen in the wardrobe and in a fight; not yet looked at in a film.

**Done when:**
- [x] Before the first scene, the player picks a colour and one accessory with the move keys, in under ten seconds
- [x] The look is one small struct that the fighter draws from
- [x] The look is stored with the recorded take, and the final cut's puppet is drawn from it
- [x] The choice survives a retake and the next scene

**Notes:** Asked for by the developer on Thu Nov 12: "allow user to simple customize his character and later show that exact character in the video", and keep it simple. Suggested accessories, each a couple of extra shapes on the stick figure: headband with trailing ends, scarf, hat, shades. No free-form editor.

Sprint: [[Sprint 2 - Three scenes]] · Stream: feel
