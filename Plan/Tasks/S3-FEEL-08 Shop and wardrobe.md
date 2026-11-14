---
sprint: 3
stream: feel
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-10 A real body]]"
---

**Goal:** The wardrobe and the shop look like parts of a game, not like lines of text.

Asked for by the developer on Fri Nov 13: "in game shop is old style and customization also old style need imprvments".

**You own:**
- `Source/StickToTheScript/UI/StickHUD.*` (how menus and the wardrobe are drawn)
- one line each in `Core/GameFlow.cpp` (how a menu row is marked; the turning fighter) and `Fighter/StickFighter.*` (a look's colour, for its swatch)

**What changed:**
- **The wardrobe** has a panel beside the fighter: every colour as a swatch with the chosen one framed, the hats and builds as a row of pips with the chosen one in gold, the chosen name large, the keys in gold. What has to be bought first is shown faint. The fighter turns slowly on the spot, so the choice is seen from every side.
- **Every menu** (title, pause, the cut to choose, the shop) draws its rows as bars with the chosen one filled gold, instead of a `>   row   <` in a list of centred lines.
- **The shop** has two columns: what it is on the left, what it costs or that it is yours on the right.

**Not done:** a picture of what is for sale. The shop still names its goods; showing the hat or the bat itself needs a small stage of its own.

**Done when:**
- [x] The wardrobe's panel shows and follows the choice: seen with two colours and two hats
- [x] The title menu, the pause menu and the shop draw their rows as bars: seen
- [x] The packaged build is made with it
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
