---
sprint: 3
stream: core
status: doing
owner: lead
estimate: 4
depends:
  - "[[S2-SHIP-02 Integration checkpoint 2]]"
---

**Board tidy, Fri Nov 13:** the developer has played three times and said what was wrong each time; difficulty and shop prices are still not tuned by hand.

**Goal:** Outside players can finish the game and want to see their final cut.

**You own:**
- `Content/Scripts/`

**Done when:**
- [ ] Three outside players have played
- [ ] The top three confusions are fixed
- [ ] Difficulty rises across the three scenes

**Retakes tire the opponents (Fri Nov 13):** takes 1 to 3 of a scene are fought at full strength. From take 4 the player takes 15% less damage with every retake, down to 40% of it from take 7 on. The take's slate says so in gold. The numbers are in `Core/GameFlow.cpp`. The gold line was seen on a fourth take on the rooftop.

**Broken arms (Fri Nov 13, asked for by the developer):** a heavy blow breaks an arm only if it leaves the fighter at 35% of its health or less; before, every kick, shot and thrown prop broke one. A broken arm does not punch: the fighter uses its other hand or its foot. The number is in `Fighter/StickFighter.cpp`.

**Rooftop, measured Fri Nov 13:** the test fighter played it for about twelve minutes over two runs and went down 23 times without a wrap. In the eleven takes of the second run it put down three opponents five times and four opponents six times, never five of the six. The test fighter never guards and only ducks a shot, so a person may do better; nobody has tried.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: core
