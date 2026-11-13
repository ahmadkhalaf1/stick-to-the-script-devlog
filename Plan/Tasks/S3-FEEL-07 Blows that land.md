---
sprint: 3
stream: feel
status: done
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-09 Jab and cross]]"
---

**Goal:** A blow that lands looks and sounds like one.

Asked for by the developer on Fri Nov 13, on playing the new punch: "make it feel like a real hit and make sound effects and visual effect maybe if needed", and of the choices offered, "Does not look like a hit".

**You own:**
- `Source/StickToTheScript/UI/ImpactEffects.*`
- one number in `Source/StickToTheScript/Cam/FightCamera.cpp` (the jolt of a light blow)

**What a landed blow already had:** a freeze of a twentieth of a second, drops of blood, a recorded sound, a small jolt of the camera, the struck figure flashing to a skeleton, and a hit reaction. What it lacked was anything at the point of the blow itself.

**What it has now, on every blow, in play and in the film:**
- A burst of light where it lands: a bright core and eight rays that shoot out of it, for a tenth of a second of real time, larger for a heavy blow.
- Yellow sparks thrown back at the attacker, as a blocked blow already had.
- The freeze of a light blow nearly doubled, from 0.05 to 0.09 seconds, so the fist is seen where it landed.
- The recording of the blow played a quarter louder, with the thump the game makes in code under it for weight.
- A stronger jolt of the camera on a light blow, 0.8 where it was 0.55.

**Done when:**
- [x] The burst shows on a landed blow: caught in two of fourteen pictures of a fight on self-play in the editor
- [x] It reads as a flash and not as an object: the first version was a pale ball nearly a metre wide that hid the fighters; it is now a small core with rays
- [x] The sound of a blow is right. The agent cannot hear it; the developer was asked to play with the sound on and gave the verdict below, without a word on the sound itself
- [x] The developer has played it and finds a punch feels like a hit (Fri Nov 13, 19:29): "Feels like a hit"

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
