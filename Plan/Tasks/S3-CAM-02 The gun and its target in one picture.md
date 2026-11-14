---
sprint: 3
stream: cam
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-11 Guns]]"
---

**Goal:** Whoever a gun is fired at is in the picture, in the fight and in the film.

Reported by the developer on Sat Nov 14, after watching a self-played run and its film: "your playtest is shooting before the enemy is in view , i dont see where and what is he shooting at", and of the film: "we see him shooting but where?"

**What was wrong:** the camera travels with the player and sees about 3.2 m to either side on the middle line of the set. A gun aimed itself at the nearest opponent up to 11 m away, and the game's own player fired from 7.5 m. So most shots were fired at someone outside the picture. In the film the wide camera had the same view, and every hit by a shot was cut to a close-up of whoever was hit, which showed no shooter at all.

**What is there now:**
- With a gun in the hands of the fighter it follows, the camera (`Cam/FightCamera`) finds who the gun points at, the same way the fighter aims it: the nearest opponent on its feet, roughly ahead, within 9.5 m. It slides to the middle of the two and pulls back, by up to 3.8 m, until both are in the picture with room to spare. It stays on that for 0.7 s after, so a shot is seen to arrive.
- It works from joints and from where the gun is, not from what the fighter knows, so the film gets the same picture from its puppets.
- A gun now aims at nobody further than 9.5 m (it was 11 m), which is what the camera can hold.
- The game's own player holds a gun for 0.6 s before its first shot, the time the camera takes to get there.
- In the film a hit by a shot is no longer a close-up. It is watched from the wide camera, shooter and target together; only when the shot puts someone down does the film cut in, a quarter of a second after, for the fall.
- The lamp stands of the warehouse and the rooftop stood just behind the camera's track; they are moved back behind where it can now go (`Tools/build_stage01.py`, `Tools/build_stage03.py`).

**You own:**
- `Source/StickToTheScript/Cam/`

**Done when:**
- [x] With a pistol picked up, the player and the opponent it points at are both in the picture: seen in the editor in the warehouse
- [x] The game playing itself: at each of eight shots the shooter and the one hit were both in the picture
- [x] The film shows shots from the wide camera with both in it: seen
- [ ] The same in the bar and on the rooftop. Rooftop: seen in the packaged game's recording of Sat Nov 14, 18:15, a rifle shot with its target in the picture (`Docs/store/3-rooftop.jpg`). Bar: still not seen, no shot was fired there in that run
- [x] The packaged build has it: in its recording of Sat Nov 14, 18:15, the pistol in the warehouse and the rifle on the rooftop are fired with their targets in the picture. Before that: packaged at 14:14 and run without a window: a wrap, a film, 16 shots, no errors. Its picture has not been looked at
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: cam
