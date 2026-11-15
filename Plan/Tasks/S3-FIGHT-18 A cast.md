---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 3
depends:
  - "[[S3-CAM-04 The film flows]]"
---

**Goal:** What the developer said on Sat Nov 14 of the fighters, "its look boring now , no skin , no face", and on Sun Nov 15 of faces and gear stuck on the dummy, "this still look ugly , and not 2026 like with AI , try to do better". Characters with skin, faces and clothes, in the dummy's place.

**You own:**
- `Tools/build_cast.py` (new), `Content/Characters/Cast/`
- `Source/StickToTheScript/Fighter/DummyBody.*`, `Fighter/StickFighter.*`, `Fighter/StuntPartner.cpp`, `Core/StickGameMode.cpp` (two lines), `Config/DefaultGame.ini` (one line)

**Where they come from:** Mixamo (Adobe), the same source as the dummy and the fight clips, free with an Adobe account and royalty-free in a game. The developer signed in; the agent chose and downloaded seven in the developer's browser on Sun Nov 15, with the developer's leave ("Yes, download them"): Ch06_nonPBR for the hero, Bryce (Ch42_nonPBR) for a thug, Michelle (Ch03_nonPBR) for a rusher, Kachujin G Rosales for a kicker, Big Vegas for a heavy, Swat for a gunman, The Boss for the boss.

**How they fit:** the game still works on sixteen joints. `DummyBody` already placed the dummy's bones on them by name, and every Mixamo character has the same bone names, so a character is put on a fighter by swapping the model. Each keeps the materials and pictures it came with.

**What had to change:**
- A character is sized by how high its hips stand, not by its legs: Big Vegas, with short legs under a long body, came out half as big again as everyone else.
- A limb is drawn no more than a third longer than it was made: a knocked-out body's joints lie far apart, and Michelle's leg was a rubber hose across the floor.
- A limb that comes off is still a plain shape, in the colour of the sleeve or trouser leg it was in.
- The gun in a gunman's hand was a black box. It is the pistol prop's own model now. (The developer saw it: "that solder gun at roof scene is fucked up".)
- The hero's clothes are black, and the bar and the roof are dark. He has a small light of his own that goes with him, in the fight and in the film.

**Done when:**
- [x] All seven are in the game and fight in their own skins: photographed in all three scenes
- [x] Big Vegas is the size of a man, and nobody lies on the floor as a hose
- [x] The gunman holds a pistol: seen on the roof, at his hip and aimed
- [x] The hero can be seen in the bar and on the roof, in a light of his own
- [x] The packaged build has them, textured and not grey (seen in the recording of it), and plays itself to the end card
- [x] The store page and the dev log's register name all seven
- [ ] The developer has seen them in the game

**Known and left:** the hero wears his own clothes, so the wardrobe's colour is now the colour of the light he is filmed in. Its hats were sized for the dummy's head; photographed on his on Sun Nov 15, they sit on top of the cap he comes with. The captive is still the dummy.

**Balance, the same night:** opponents have a fifth more health, the finish bar takes about twenty blows, the finish reaches 2.6 m and only hurts a heavy. The game's own player went from 14, 17 and 25 seconds a scene to about 19, 28 and 34.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
