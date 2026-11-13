---
sprint: 1
stream: cam
status: done
owner: lead
estimate: 4
depends: []
---

**Goal:** First-person view from the player's chest, with slight lag and shake, and the player's own arms in frame.

**You own:**
- `Source/StickToTheScript/Cam/BodyCam.*`
- `Content/Camera/`

**Done when:**
- [x] Play-In-Editor shows the body-cam view
- [x] A punch visibly enters the frame

**Notes:** No lens effects yet: those are S2-CAM-03. Can start now against the existing AStickFighter.

- Worn at eye level, not on the chest: from the chest the player's own guard is above the frame.
- It keeps looking at the spot where the opponent stands, so a lunging head does not lose them.
- **No longer the main view.** On Thu Nov 12 the developer decided the fight is played from outside. The body cam stays as a second angle on the V key and for point-of-view inserts in the final cut.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: cam
