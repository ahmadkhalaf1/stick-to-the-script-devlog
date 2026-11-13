---
sprint: 1
stream: fight
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-FIGHT-01 Fighter move set]]"
---

**Goal:** Two defensive moves that read clearly from the side and from first person.

**You own:**
- `Source/StickToTheScript/Fighter/`
- `Content/Characters/Anims/Defence/`

**Done when:**
- [x] Block and duck both work through PlayMove
- [x] Their source is in the asset register (made in code by the team, so there is no asset to register)

**Notes:** Risk: the engine ships no defensive animations. Try in this order: licensed motion capture retargeted to the mannequin, then a procedural pose (bone rotations blended in code).

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: fight

Both are posed in code on the stick joints, over whatever animation is playing: block raises both forearms in front of the face, duck drops the hips 45 cm, leans forward and bends the knees with two-bone IK. No motion capture was needed.
