---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-18 A cast]]"
---

**Goal:** What the developer said on Sun Nov 15, the cast being in: "re-think of character customization now since we use different char make sure it works and looks good", and "re check broken limbs and how it looks a the new character and improve it". The wardrobe and the wounds were made for a plain figure. Both have to suit a character with a skin and clothes.

**You own:**
- `Tools/build_cast.py`, `Content/Characters/Cast/M_CastCloth`, `Content/Characters/Cast/Hero/` (the model's material slots and two new materials)
- `Source/StickToTheScript/Fighter/DummyBody.*`, `Fighter/StickFighter.*`
- `Source/StickToTheScript/UI/StickHUD.cpp` (the rim round a swatch), `Core/StickPlayerController.*` (a test key)

**The wardrobe:**
- Colour: the hero's hood, cap and trousers are black and plain in his pictures, and nothing else in them is but his eyes, brows and hair, which sit in one corner of one picture. `M_CastCloth` swaps what is dark and without colour for a tint, outside that corner. The hood and the cap take the colour, the trousers a dark shade of it. The first colour is now "Black": no dye, the clothes as they were made.
- Hat: three of the six went round a bare ball of a head and were lost in a modelled one with a cap and headphones on it. They are a bowler, a wide brim and a party hat now, which sit on top of the cap as the top hat always did. A hat can be a cone or a ball as well as a cylinder or a cube.
- Build: unchanged. It makes the model stouter or slighter, as it did the dummy.

**The wounds:**
- A limb that comes off one of the cast is that limb: a second copy of the model with everything but the forearm and hand, or the shin and foot, closed down to nothing. It was a plain cylinder in the colour of the sleeve.
- What is left ends in a flat cut as wide as the limb was, with a red face and the end of the bone in it. It ended in a point with a red ball on it.
- A broken forearm cannot be drawn snapped in two on a model, whose forearm is one bone. The wound goes round the middle of it, the bone comes out of it aslant, and the hand hangs off sideways with its fingers open. The bone and the wound used to float beside the arm, where the plain figure's kink was.
- A neck is drawn no longer than a neck can be, as a limb already was: a body thrown into the air by the finish had one a forearm long.
- The wounds are put on the body as it was drawn, not on the joints, which a stretched limb stops short of.

**Done when:**
- [x] Every colour of the wardrobe dyes the hero's clothes and leaves his skin, shoes and headphones alone: photographed
- [x] Every hat sits on his head and reads as a hat: photographed. The crown reads least
- [x] Cut limbs and broken arms photographed on the hero and on opponents in all three scenes: the thug and the kicker. Not the rusher, the heavy, the gunman or the boss
- [x] The packaged build has the dye and the wounds, and plays itself to the end card: run and its log read, not seen
- [ ] The developer has seen them in the game

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
