---
sprint: 3
stream: feel
status: done
owner: lead
estimate: 4
depends:
  - "[[S3-FEEL-02 Look pass]]"
---

**Goal:** The figures stop reading as placeholder sticks and read as stunt dummies, without losing a single move.

Asked for by the developer on Fri Nov 13: "do u recommend using better character than the stick figure? maybe sth more modern or nicer in 2026". The agent advised against a different character: every move beyond three animations exists only as positions of the stick's 16 joints, and so does the film, so a skinned character would lose all of it. A better body on the same joints keeps everything. The developer: "ok".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*` (the drawing: `PoseStick`, `SetColor`, `TintBody`)

**Done when:**
- [x] A chest, a waist and hips round the spine; a boot on each foot that follows the leg through a kick
- [x] Limbs of different thickness: thigh over calf, upper arm over forearm
- [x] Joints, fists and boots in black rubber (steel on a figure that is itself near black)
- [x] A crash dummy's yellow and black marks on the chest and on the outside of each shoulder (not on the head: seen from the side, a mark on the temple reads as a huge eye)
- [x] The skeleton flash, the broken arm, the hats, the wardrobe's builds and the film all still work (seen: the skeleton flash, the film, and a crown in the wardrobe. A broken arm and the other builds use code that did not change and were not looked at)
- [x] Seen in play in all three scenes, and in the warehouse's film
- [x] The developer has seen it: "i like it keep i"

**How it works:**
- Everything is placed in `PoseStick` from the same 16 joints, so the film's puppets get it too.
- `BodyParts` are spheres stretched into shape; `Marks` are flat discs. All sizes come from the figure's `Thickness` and `HeadRadius`, so the wardrobe's builds and the opponents' kinds keep their own silhouettes.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
