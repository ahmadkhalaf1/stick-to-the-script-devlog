---
sprint: 3
stream: stage
status: doing
owner: lead
estimate: 3
depends:
  - "[[S3-FIGHT-10 A real body]]"
---

**Goal:** The things a fighter picks up and throws behave like things: they arrive, bounce, break and are thrown with the body.

Asked for by the developer on Fri Nov 13: "throwing items also need improvbment in quality and graphic and spawning and holding and throwing techinqes".

**You own:**
- `Source/StickToTheScript/Fighter/ThrownProp.*`, the throw's pose in `Source/StickToTheScript/Fighter/StickFighter.cpp`
- `Tools/build_stage01.py` (the warehouse's light and haze)

**What changed:**
- **Arriving.** As a take is called, everything on the set drops into its place from a little way up and bounces to rest. Before, it was simply there.
- **Landing.** A prop that comes down fast bounces, lower each time and with less spin, then rights itself over a moment. Before, it stuck where it touched and snapped upright.
- **Breaking.** A bottle, a melon, a worn-out bat or a spent golden prop leaves ten pieces in its own colour that fly apart, land and lie there until the next take. Before, it vanished. The film breaks it again: it notices the recorded prop dropping out of the world.
- **Throwing.** The body leans into the throw as the arms come over, and straightens again. Before, only the arms moved.
- **The warehouse** has a weaker flat light from above and thicker haze, so the lamps and windows make pools and shafts.

**Not done, and why:**
- A pick-up, a hold and a throw from motion capture. They need new clips downloaded from Mixamo in the developer's browser, and a download needs the developer's word.
- Better models for the props themselves. Not looked at yet.

**Done when:**
- [x] A crate drops in and bounces as "action" is called: seen in the bar
- [x] A throw leans the body: seen in four slow-motion pictures
- [x] A bottle breaks into pieces in play and again in the film: both in the log, once each. The pieces are too small to make out in the test's pictures, so how they look has not been judged
- [x] The packaged build is made with it. It ran for a minute with no errors, at nine frames a second, with another game running on the PC and its own window not in front: no measure of anything
- [x] How a held bat sits in the model's hands has been looked at: bought, picked up and swung on Nov 14. Both hands on the thin end, the bat over the right shoulder
- [x] A prop put on a counter comes to rest on it. Found broken on Nov 14: since props drop in at the start of a take they fell through to the floor. Fixed (`bDroppingIn` in `Fighter/ThrownProp`), and the bottles were seen on the bar
- [ ] The developer has played it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: stage
