---
sprint: 2
stream: feel
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-FEEL-02 Placeholder sound set]]"
  - "[[S1-CAM-03 Final cut player v1]]"
---

**Board tidy, Fri Nov 13:** the film has its music and its blows; recordings came in [[S3-FEEL-04 Recorded sound]].

**Goal:** The replay sounds like a film: hits on the cut, music, a slow-motion treatment.

**You own:**
- `Content/Audio/`
- `Source/StickToTheScript/UI/`

**Done when:**
- [x] Hit sounds are driven by the recorded events
- [x] Music made in code, like the other sounds: a bare pulse under the fight and the whole band, faster, over the film
- [x] One music cue per scene: the fight's music is a recording per scene (`M_Fight_1` to `M_Fight_3`, found by the scene's number in `UI/FilmScore.cpp`). The film's music is one for all three
- [ ] Somebody has listened to it. The agent cannot hear: it only knows that sound reaches the mixer
- [x] A whoosh on every swing in the film. Since Sat Nov 14 the recorder keeps every swing, not only a gun going off, and the film fires them again. One action cut of the warehouse: 14 strikes and throws swung again with 12 whooshes, 2 spinning ones and 3 shouts, where there were none; the 8 shots as before. By the log: nobody has listened
- [x] Every source is in the asset register

Sprint: [[Sprint 2 - Three scenes]] · Stream: feel
