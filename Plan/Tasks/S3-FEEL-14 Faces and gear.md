---
sprint: 3
stream: feel
status: dropped
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-13 The film on a screen]]"
---

**Goal:** What the developer said on Sat Nov 14: "i still think we can improve character a bit more , its look boring now , no skin , no face , what do u thnk?" The agent laid out two ways, faces and gear on the body there is, or textured characters with faces from Mixamo, and recommended the first. The developer chose it: "Faces and gear".

**Dropped on Sun Nov 15:** built, photographed and shown, and the developer turned it down: "this still look ugly , and not 2026 like with AI , try to do better". It is on the branch `feel/S3-FEEL-14-faces-and-gear` and not in the game. Boots were built too and taken out before that: over the model's feet they looked like weights. Textured characters are the way on.

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*` (the face, the gloves, the boots, what a kind wears), `Fighter/StuntPartner.cpp` (one line: a kind dresses itself)
- `Content/Scripts/Scene03.json` (the boss's colour)

**What it is, all of it plain shapes placed from the joints, as the hats are, so the film shows it too:**
- A face on every fighter: two eyes with pupils, and brows. Opponents scowl, each kind by its own amount. A blow opens the eyes wide and lifts the brows for half a second. A knockout crosses the eyes out.
- A glove on each hand and a boot on each foot: red for the player's fighter, black for an opponent. A limb that has come off takes its glove or boot with it.
- Something on the head for each kind of opponent: a woollen cap for a thug, goggles for a rusher, a topknot for a kicker, a steel helmet for a heavy, a wide-brimmed hat for a gunman.
- The boss wears black, a black hat and a red tie, and his eyes glow red.
- The player's wardrobe is as it was: colour, hat and build.

**Done when:**
- [ ] The face reads in the fight, from the side, and in the film's close shots: photographed
- [ ] A blow and a knockout show on it
- [ ] Every kind wears its own, the boss included
- [ ] The gear does not hang in the air after a limb comes off or in the skeleton flash
- [ ] The game still plays itself through all three scenes, and the packaged build has it
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
