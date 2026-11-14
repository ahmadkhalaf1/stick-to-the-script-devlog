---
sprint: 3
stream: stage
status: doing
owner: lead
estimate: 1
depends:
  - "[[S3-SHIP-02 Bug bash and feature freeze]]"
---

**Goal:** A fighter backed into the end of the rooftop behind the start is lit like anywhere else on it. Until now he stood outside every light there and was a dark shape.

**You own:**
- `Tools/build_stage03.py`
- `Content/Scenes/Stage03.umap` (generated)

**What was done:** the roof's work lights come in pairs every 5.5 m, a warm one from the camera's side and a cool one from behind. The first pair aimed at x 100, and its light ends about x -280; the arena goes on to x -350. One more pair, aimed at x -450, covers that end.

**Done when:**
- [x] The floor and the parapet at that end are lit: the set photographed from two places before and after
- [x] The game's own picture of the start is not washed out by it: the stair house reads as a lit wall
- [ ] A fighter seen standing at that end in a take
- [x] The packaged build has it: packaged on Sat Nov 14 at 20:05 with the rebuilt level. Its picture has not been looked at

Sprint: [[Sprint 3 - Feel and lock]] · Stream: stage
