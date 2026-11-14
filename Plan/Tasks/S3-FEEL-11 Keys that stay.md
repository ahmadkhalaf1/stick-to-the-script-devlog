---
sprint: 3
stream: feel
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-06 Usual keys]]"
  - "[[S3-FIGHT-12 Keys that do not block]]"
---

**Goal:** A player always sees what each key does at this moment, without it getting in the way of the fight.

Asked for by the developer on the morning of Nov 14: "find a nice way to keep key bind help visible at the game to help knowing what to press now mouse click only punsh".

**What was there:** two lines of text along the bottom, shown as a take is called and for 14 seconds of a scene's first take, then gone. Every combination was spelled out in the second line.

**What is there now:** a strip of key caps along the bottom edge for the whole take (`DrawKeys` in `UI/StickHUD.cpp`). Each cap has the key on it and what it does beside it, and that changes with the moment:
- duck held: the click is an uppercut and F a sweep; guard held: a hook and a roundhouse
- a gun in the hands: the click fires, or says the gun is empty; a bat or a hammer: it swings; anything else: it throws
- in the air F is a flying kick; on a bar the click and F are the swing kick and Space lets go
- E says "Pick up" when a prop is in reach (`FindPropInReach` in the fighter, the same search the grab uses), and "Throw" with something in the hands

A meaning that differs from the plain one is in gold. A key that is down lights its cap. One line above the strip, as a take is called and for the first seconds of a scene, says what the strip cannot: hold guard or duck and strike, jump under a red bar.

**Left out:** a key to hide the strip. The keys are still written in the HUD by hand, not read from the bindings.

**Done when:**
- [x] The strip shows for the whole take and not in the film or over a card: seen in the editor
- [x] Its words change with duck held, with a pistol in the hands, in the air and with a prop in reach: seen, with the duck key really held
- [x] The packaged build has it
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
