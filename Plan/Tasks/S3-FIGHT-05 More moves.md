---
sprint: 3
stream: fight
status: done
owner: lead
estimate: 6
depends:
  - "[[S2-FIGHT-04 Free fight]]"
---

**Board tidy, Fri Nov 13:** every box ticked; not played by hand by the agent.

**Goal:** More to fight with than a punch and a kick, without one more key to learn.

Asked for by the developer in the night of Thu Nov 12 to Fri Nov 13: "try to improve fighting animation and try to introduce more moves and maybe a climb jump hit where character can use bars to jump and hang at it then jump kick the enemy , just be creative add new moves or karate moves , animation and sounds keep in mind".

**You own:**
- `Source/StickToTheScript/Fighter/`

**Done when:**
- [x] Ducked, a punch is an uppercut that throws its victim into the air
- [x] Ducked, a kick is a sweep that takes the legs from everyone round the fighter
- [x] Behind a raised guard, a punch is a karate chop that stuns and goes through a guard
- [x] Behind a raised guard, a kick is a spinning roundhouse that goes through a guard and over a ducked head
- [x] The plain kick lifts its knee before the foot goes out
- [x] A fighter waiting for an opening is not standing still, and a landing has weight (in the code; too small to pick out in a still frame)
- [x] Each new move has its own sound, and the box office pays for each as its own kind (the sounds are synthesised and nobody has heard them)
- [x] The take's slate and the pause menu say how the moves are done (written; not looked at on screen yet)
- [x] Hanging from a bar and kicking off it: see [[S3-FIGHT-06 Bars]]
- [x] The opponents use them too, each kind its own: a thug chops, a rusher sweeps, a kicker throws the roundhouse, the heavy has the uppercut. A bigger move is shown a third longer before it lands, and holds the player helpless for half as long as it holds an opponent (Fri Nov 13; not played by hand)

**How it works:**
- The engine's mannequin brings three attack animations and nothing else, so every new move is posed in code on the sixteen joints, the way the kick already was. `FTechnique` in `Fighter/StickFighter.cpp` is the table of them: how long each takes, what it hits, what it does to who it hits.
- No new keys. `Strike` looks at what the fighter is holding: a duck or a raised guard turns the plain punch or kick into the other move. `StrikeWith` asks for one by name; the test fighter uses it.
- The moves are new values of `EMove`, so everything that hears `OnHit` and `OnSwing` can tell them apart: the sounds, the box office, the film.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
