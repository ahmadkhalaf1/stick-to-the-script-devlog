---
sprint: 1
stream: cam
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-CORE-01 Shared types and events]]"
---

**Goal:** Every fighter's joint positions and every event of a take are recorded, including failed takes.

**You own:**
- `Source/StickToTheScript/Cam/TakeRecorder.*`

**Done when:**
- [x] After a take, FTake holds one frame per tick for each fighter
- [x] Memory used by a 60-second take is logged (4.2 MB for two fighters)
- [x] An automation test records ten frames of a known pose and reads them back

**Notes:** A stick figure is about 16 joints, so store positions, not animation state.

- The class is `AFightRecorder` in `Cam/FightRecorder.*`. The engine already has a header called `TakeRecorder.h`, and Unreal refuses two headers with one name.
- Each frame also stores which arms are broken, each hit stores where it landed and whether it knocked out, and the take stores every fighter's colour. A replay needs all three.
- Failed takes are recorded too, but only the last one is kept. The blooper reel (S2-CAM-02) needs a list.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: cam
