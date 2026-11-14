---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 5
depends:
  - "[[S3-FEEL-05 Stunt dummy look]]"
  - "[[S3-FIGHT-07 Real fight animations]]"
---

**Goal:** The fighters are a modelled character, not balls and tubes, without one pose of the game being lost.

Asked for by the developer on Fri Nov 13, late: "i still feel like we need to improve graphic and character alot to fit a game in 2026". The agent agreed that the character was the largest gap, said it was the riskiest job left, and set itself a cut-off at which the old dummies would come back.

**You own:**
- `Source/StickToTheScript/Fighter/DummyBody.*` (new), `Source/StickToTheScript/Fighter/StickFighter.*`
- `Content/Characters/Dummy/` (new: the model), `Tools/build_character.py` (new)

**How it works:**
- The model is the one that comes with the Mixamo clips, Adobe's "X Bot": a smooth robot dummy with hands, feet and segmented joints. `Tools/build_character.py` takes it out of one of the downloaded FBX files, with no animation and no material of its own.
- The game does not animate it. The sixteen joints stay the truth: every frame `UDummyBody` puts the model's bones on them. So the poses worked out in code (duck, guard, carry, hang and swing, recoil, a broken arm) and every take replayed in the film show on it without knowing it is there.
- Elbows and knees bend about the axis the two bones make; a straight limb leans on a guess of which way it would bend. Hips and chest keep their shape round the hip and shoulder joints, and the spine takes up the difference between this model's back and the mannequin's. A foot lies flat under a standing leg and follows its shin otherwise. The fingers are closed into fists.
- It wears the figures' own material: the fighter's colour on the shell, rubber on the joints. A stouter or slighter build swells or thins it.
- The skeleton that shows through at a blow is still the old plain shapes, and they are the fallback: without the model in the project the fighters are drawn as before.

**Done when:**
- [x] The model stands, walks, strikes and falls on the joints in all three scenes, seen in the editor on self-play
- [x] The film shows it, close-ups included
- [x] Colours, a hat, the gunman's gun and the skeleton flash work with it. Seen: four colours and Lime in the wardrobe, a top hat. Not looked at: the other hats, a broken arm, the stout and slight builds
- [x] The packaged build shows it, in colour
- [x] It costs no frame rate that matters. Measured again on Nov 14 at 04:45 with nothing else running, and with the scanned sets in as well: the packaged game played the warehouse at a median of 96 frames a second (lowest 89), the bar at 106 (lowest 103) and the rooftop at 104 (lowest 102). The 89 of the night before was the other game running beside it
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
