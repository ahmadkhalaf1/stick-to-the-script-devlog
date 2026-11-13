---
sprint: 3
stream: fight
status: done
owner: lead
estimate: 1
depends:
  - "[[S3-FIGHT-07 Real fight animations]]"
---

**Goal:** The punch, the move thrown most, is motion capture from the same stance as the other moves.

Proposed by the agent on Fri Nov 13 when the developer asked what still needs improving; the developer: "Yes, punches".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.cpp`
- the word for the guard-and-punch move, wherever the game and the store page say it

**How it works:**
- `A_JabCross`, converted on Fri Nov 13 and unused until now, is the punch of a fighter with two sound arms and empty hands. The jab measures the distance; the cross is the blow that counts, found in the clip like every other contact (0.67 s, 99 cm, right hand).
- A punch that has to arrive fast shows only the end of the clip, so the player's punch is the cross with a step in.
- With an arm hanging loose, or a weapon in the hands, the punch is the engine's as before: those pick a sound arm and swing what is held.
- The guard-and-punch move has looked like a hook since the clips went in. The game and the store page now call it a hook. In the code it is still `EMove::Chop`.

**Done when:**
- [x] A punch on the ground is the cross: seen in six pictures of one punch in slow motion in the editor
- [x] It still lands: three punches in the log, two of the player's and one of an opponent's, the fist of the last two stopping 33 and 18 cm from the body. The first was thrown from a jump and stopped 74 cm away, over the opponent's head, which is what a punch from the air did before too
- [x] A punch with a broken arm, and with a weapon in the hands, still works. Read in the code, which sends both to the punches the game had before; not tried
- [x] "hook" on the line of keys, the pause card, the pay line and the store page's text
- [x] The developer has played it (Fri Nov 13): "Needs work", and asked what: "make it feel like a real hit and make sound effects and visual effect maybe if needed", "Does not look like a hit". Taken up in [[S3-FEEL-07 Blows that land]]

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
