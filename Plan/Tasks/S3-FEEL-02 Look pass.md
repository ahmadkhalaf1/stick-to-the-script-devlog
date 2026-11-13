---
sprint: 3
stream: feel
status: done
owner: lead
estimate: 6
depends:
  - "[[S2-FEEL-03 Character look]]"
---

**Goal:** The game looks good enough that the first thing a player sees is not a set of cylinders. Asked for by the developer on Thu Nov 12: "polish and improve graphics and assets and character, animation, make the game look nice".

**You own:**
- `Source/StickToTheScript/Fighter/` (how a figure is drawn and how it stands)
- `Tools/stage_kit.py` (the picture every set shares)
- `Source/StickToTheScript/UI/` (text)

**Done when:**
- [x] Figures have a ball at every joint, hands and feet, a thicker trunk, and eyes that show which way they face
- [x] A fighter standing still keeps its hands up instead of letting them hang
- [x] Every set has the same film look: glow on lights, grain, a little colour fringe, stronger contrast and colour
- [x] Text is sharp: drawn from the engine's outline font at the size asked for. The first try drew nothing, because the engine's canvas text refuses to draw without a font object attached even when it is handed an outline font; an empty one is now attached
- [x] Opponents who back away walk backwards instead of jogging in reverse (in the code; not yet picked out in a frame)
- [x] Props look like things: straps on crates, a neck and a label on a bottle
- [x] Funnier things to throw: a watermelon that bursts, a traffic cone, a frying pan
- [x] A struck figure shows as its own skeleton for a moment (the developer's idea, in place of a white flash; not yet seen by the agent)
- [x] A break shows as a kink with a pale bone spike and a wound, and the arm keeps its colour
- [x] The title card is a title: five short lines and a red rule, with the set showing through
- [x] The developer has looked at it and said what still looks cheap: on Fri Nov 13, "the game feel poor". What came of it is [[S3-STAGE-03 Surfaces and set dressing]]

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
