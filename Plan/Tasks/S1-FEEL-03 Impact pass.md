---
sprint: 1
stream: feel
status: done
owner: lead
estimate: 6
depends:
  - "[[S1-FIGHT-01 Fighter move set]]"
---

**Board tidy, Fri Nov 13:** blood, sparks, frozen frames and the slow knockout are all in.

**Goal:** Every hit is felt: the body reacts where it was struck, and the screen and the speakers react with it.

**You own:**
- `Source/StickToTheScript/Fighter/` (how a body reacts)
- `Source/StickToTheScript/UI/`, `Content/Audio/`, `Content/Materials/` (what the player sees and hears)

**Done when:**
- [x] A hit throws a burst of blood from the point of contact, and leaves marks on the floor
- [x] A heavy hit can break a limb: it hangs and swings loose for the rest of the take
- [x] The struck part of the body reacts, not only the whole figure
- [x] The camera shakes on a hit, and a heavy hit freezes the frame for a moment
- [ ] Every strike, block and knockout has a sound (built and reaching the audio mixer, but only the developer can hear whether it is right)
- [x] All of it is driven by `OnHit` and the new `OnBlocked`, so the final cut replays it for free

**How it was built:**
- Blood and sparks are small spheres from a pool of 260, moved in code. Blood that lands flattens into a mark and stays until the pool comes round to it again.
- A heavy hit that does not knock out breaks the arm nearer the impact. The forearm becomes a pendulum hung from the elbow, drawn in two dark red pieces at an angle. It mends at the next take.
- A hit pushes each joint by how close it is to the point of impact, so a punch to the head snaps the head back.
- A freeze-frame is the whole game slowed to a twentieth for a few hundredths of a second. A knockout plays at a fifth of normal speed for most of a second.
- Sounds are synthesised in `UImpactSynth` from sine sweeps and filtered noise. There are no audio assets.
- Still open: only arms break, the partner's later punches ignore his broken arm, and the knocked-out partner can land outside a narrow window.

**Notes:** Added on Thu Nov 12 after the developer changed direction: the fight is watched from outside, and the time the body cam look would have taken goes here. On stick figures this stays cartoonish. Sounds must be made by the team or licensed, and listed in the dev log's register.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: feel
