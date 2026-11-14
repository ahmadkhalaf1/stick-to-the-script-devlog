---
sprint: 3
stream: cam
status: doing
owner: lead
estimate: 3
depends:
  - "[[S3-CAM-01 Final cut polish]]"
  - "[[S3-CAM-02 The gun and its target in one picture]]"
---

**Goal:** Nothing in the film is cut off, half finished or hidden.

Asked for by the developer on Sat Nov 14: "also review how reply videos are made and pov , make sure nth is cutted in strange way or half finished or partially visible". With it, about the guard: "i also noticed strange way of guard move , the animation and character position and hand position looks strange".

**How it was reviewed:** one film of the warehouse photographed four times a second, 118 pictures, and every one looked at.

**What was found:**
- A close shot was aimed once, at the spot of the blow, and then held. Whoever was hit was knocked out of it; three pictures in a row, most of a second, showed a wall and some blood.
- It was taken from 2.6 m and aimed at the blow, so heads were cut off by the top of the picture, and whoever struck the blow was often an arm at the edge.
- The camera on the floor was twice put behind the wet-floor sign, which filled a third of the picture.
- A film left a knockout 0.8 s after it, before the body was down.
- A hit by something thrown was a close shot of the one hit, with the thrower nowhere.

**What changed (`Cam/FinalCut`, `Cam/FightCamera`):**
- A close shot follows its people: it looks at the chest of whoever is hit, drawn half way towards whoever hit them, and turns with them for as long as it lasts.
- It is taken from 3.3 m (3 m for the floor camera), which holds two fighters and their heads.
- Before a camera is put down, the props are asked whether one stands between it and its people. If so the other side is tried, then higher up.
- A knockout is stayed with for 1.1 s, and a close shot for 1 s after its blow.
- Something thrown is treated as a shot is: watched from the wide camera, which now also makes room for whoever a carried prop would be thrown at.

**The guard (`Fighter/StickFighter.cpp`):** it held both fists together above the forehead on a body standing straight. It is a boxer's guard now: fists by the chin with the lead hand further out, elbows down in front of the ribs, knees soft.

**Done when:**
- [x] The warehouse's film again, 96 pictures: both fighters and their heads in the close shots, the camera on whoever goes down, no empty picture, the sign not in the way
- [x] The bar's film, 150 pictures, sampled: the same
- [x] The guard seen in play and in the film
- [ ] The rooftop's film, the slow-motion cut and the long take
- [ ] A fighter who is not part of a blow can still stand between the camera and it, out of focus
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: cam
