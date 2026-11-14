---
sprint: 3
stream: feel
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-10 Mouse in the menus]]"
---

**Goal:** Two things the developer asked for on Sat Nov 14: "show enemy health bar", and the first menu as cards, "a blured locked scene attached with a path style from seceince 1 to 2 to 3".

**You own:**
- `Source/StickToTheScript/UI/StickHUD.*`
- `Source/StickToTheScript/Core/GameFlow.cpp` (left and right in a menu)
- `Content/UI/`, `Tools/ui/`, `Tools/build_ui.py`

**What was done:**
- Every opponent in the fight carries a small bar over the head. The boss has the big one along the top instead.
- The first menu shows each scene as a picture card with its number, on a path of stepping stones that turns gold as far as the player has got. A scene still locked shows the same picture out of focus, says LOCKED, and names the scene to win first. Shop, Credits and Quit are rows under the cards.
- A and D move through a menu as W and S do, since the cards lie side by side. The pointer chooses a card as it chose a row.
- The six pictures are frames of the gameplay video of Nov 14; `Tools/build_ui.py` brings them into `Content/UI`.

**Done when:**
- [x] Bars over the opponents, seen in a take in the editor
- [x] The menu's cards, the chosen one, the rows under them: seen in the editor with every scene open
- [ ] A locked card seen: needs a game that has not been played, so the packaged build
- [ ] The pointer choosing a card, tried with the real mouse
- [x] The pictures are in the package: the cooked game's asset registry lists all six, and a fresh copy starts with no error about them
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
