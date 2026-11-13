---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-07 Real fight animations]]"
---

**Goal:** A fighter who catches a bar swings on it like a body on a bar, instead of hanging stiff.

Asked for by the developer on Fri Nov 13: "swing at the bar should not stay solid when he catch it bar , he should swing , add some realistic physics to it".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*`

**How it works:**
- Hanging, the body is a pendulum on the bar: one angle and one speed, a metre from the bar to the body's middle, pulled by the same gravity as a jump.
- It starts with the speed the fighter caught the bar at, so a running jump swings wide and a standing one only sways. It never starts dead still.
- It loses a little speed every second. Walking left or right pushes it, so a swing can be kept going or built up in time with it.
- The legs follow the body a moment late.
- Letting go (jump, or after 3.6 seconds, up from 2.4) throws the fighter off the way the swing was going, from where the swing had carried it.
- The kick off the bar is as it was, but starts from wherever the swing is.
- A bar runs across the set, so the swing is always left and right, whichever way the fighter faces. Turning round up there does not flip it.

**Done when:**
- [x] Caught at a run, the body swings out, comes back and goes out again: seen over fourteen pictures of one hang on self-play in the editor
- [x] The kick off the bar still lands: once in that test, by the log
- [ ] Letting go with the jump key mid-swing throws the fighter the right way. In the code, not seen: the test's hang ended in the kick
- [ ] Pushing the swing with the walk keys feels right. Not judged: self-play was holding a direction during the test, and by hand nobody has tried
- [ ] The developer has played it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
