---
sprint: 3
stream: fight
status: done
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-19 Limbs and wardrobe on the cast]]"
---

**Goal:** What was left over from the cast on Sun Nov 15, which the developer asked for with "continue with the optional items", and one thing asked for while that was being done: "i dont like that fat character the white big one , change it".

**You own:**
- `Tools/build_cast.py`, `Content/Characters/Cast/Heavy/`, `Content/Characters/Cast/Kid/`
- `Source/StickToTheScript/Fighter/StickFighter.*`, `Fighter/Captive.*`, `Core/StickGameMode.cpp`
- `Docs/store/`, `Docs/StorePage.md`

**The heavy:** Big Vegas goes. The developer chose Brute from five Mixamo characters shown in a picture: a big bare-chested man with a beard, who comes with an axe in his right hand. The axe is part of his model and does nothing; he fights as the heavy always did. `build_cast.py` now brings a part in afresh when its FBX is newer than its model, so a character is replaced by putting the new file in the old one's place.

**The kid:** was the plain dummy, painted yellow. He is Timmy now, Mixamo's cartoon boy, downloaded with the developer's leave. A model is sized by how high its hips stand, and Timmy is a third head on short legs, so at the dummy's size he stood as tall as the men. The kid is a fighter at half size. And a captive no longer crouches until he is freed: crouched, Timmy folded into a heap under his own cap. He stands with his arms up in front of his face, and drops them when the last opponent is down.

**The crown:** was a band and a rim, a yellow puck. It is a band with five points. A hat can have six pieces now and a piece can sit to the side of the head.

**Found on the way:** a gunman who lost his right forearm kept a pistol hanging in the air where his hand had been. The gun goes with the forearm.

**Asked for while the developer played the build, the same afternoon:** guns held like guns (a pistol at arm's length in both hands, a rifle with its butt on the shoulder, and both kept there through a kick), and a fist or a foot that stops at the body it lands on. `KeepOutOfBodies` in `Fighter/StickFighter.cpp` takes an opponent on his feet for an upright post and draws a limb back to where it went in.

**Done when:**
- [x] Brute fights as the heavy in his own skin: photographed in the warehouse, whole and hurt
- [x] The kid is kid-sized and stands up: photographed on the roof beside a thug
- [x] The crown reads as a crown: photographed in the wardrobe
- [x] The packaged build has all of it and plays itself to the end card: run and its log read, not seen
- [x] The gameplay video and the store pictures show the game as it is: video cut at 20:15 from the build of 20:05, pictures from the 16:01 recording
- [x] The store page and the dev log's register name Brute and Timmy, and no longer Big Vegas
- [x] The developer has seen them in the game: played the builds that evening and sent screenshots of what was still wrong, which was put right

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
