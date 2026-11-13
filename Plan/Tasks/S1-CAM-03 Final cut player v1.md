---
sprint: 1
stream: cam
status: done
owner: lead
estimate: 6
depends:
  - "[[S1-CAM-02 Take recorder]]"
---

**Goal:** A clean take replays from outside as an edited scene.

**You own:**
- `Source/StickToTheScript/Cam/FinalCut.*`

**Done when:**
- [x] Puppet stick figures are drawn from the recorded joints (the fighters themselves are posed, so the film shows the same characters)
- [x] Three fixed cameras, with a cut to the close-up on every hit
- [x] Slow motion on the last hit
- [x] The game continues after the replay (the same scene is shot again until the game flow exists)

**Notes:**
- The film fires the recorded hits and blocks again as events, so blood, shake, freeze-frames, slow motion and sound come back without any replay code of their own.
- The edit: wide shot by default; a side close-up from just before each strike to just after; heavy hits and the knockout from a low camera looking up. Strikes close together share one run of close-ups.
- Cameras are found by the tags `Cam_Wide`, `Cam_Close`, `Cam_Low`, or placed around the fight if a level has none.
- Still open for S2-CAM-01: the low camera cuts heads off on some hits, there is no body cam insert yet, and the swing has no whoosh in the film.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: cam
