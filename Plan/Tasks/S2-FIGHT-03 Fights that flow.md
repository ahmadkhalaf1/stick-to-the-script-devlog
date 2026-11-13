---
sprint: 2
stream: fight
status: done
owner: lead
estimate: 8
depends:
  - "[[S2-CORE-01 Scene scripts as data]]"
---

**Goal:** A fight is one continuous run through a space, not three opponents standing on marks waiting their turn.

**You own:**
- `Source/StickToTheScript/Fighter/` (movement, blending, entrances)
- `Source/StickToTheScript/Cam/` (a camera that travels with the fight)
- `Tools/build_stage*.py`, `Content/Scripts/` (longer sets and scenes that use them)

**Done when:**
- [x] The player's fighter travels: between beats it runs or dashes to the next opponent instead of turning on the spot
- [x] Opponents enter when their part starts (through a door, from the next room), and those who are down stay where they fell
- [ ] Moves cross-fade instead of snapping, and a fast strike leaves a smear
- [x] The gap between beats is short enough that the fight never stops, and the scene has more, shorter exchanges
- [x] The playing camera travels sideways with the player, and pushes in for a one-on-one
- [ ] The last opponent of a scene is a duel: closer, longer, with a stagger before the fall

**Closed Thu Nov 12, 16:33, with two boxes open.** The cross-fade and the smear are in the code but the smear was never seen in a frame, and the closing duel is closer but has no stagger before the fall. The developer then replaced scripted beats with free fighting, [[S2-FIGHT-04 Free fight]], which keeps everything built here (travel, entrances, cross-fade, the travelling camera, the long set) and takes over the open boxes.

**Notes:** Asked for by the developer on Thu Nov 12, with a reference: the Flash animation "Xiao Xiao No. 3" (stick figure fighting), watched as still frames by the agent. What to take from it, as ideas and not as content to copy:

- A pure side view of a building in cross-section. The hero moves through rooms, up stairs and through doors, and the fight moves with him.
- Opponents come a few at a time, from doors and from the next room. Each lasts one to three moves. The bodies stay on the floor, so the room shows how far the fight has come.
- The hero is never idle: the end of one exchange is the run-up to the next.
- Variety comes from flying kicks, throws, a weapon picked up from the floor, and one opponent with a gun.
- Fast strikes are drawn with a smear. The camera is wide for the crowd and close for the final duel, which is slower and heavier.

This changes what a scene is. A beat needs a place as well as an opponent, and a stage becomes a path. It fits the rescue story in [[S2-CORE-04 The story - a rescue]]: fight through a building to reach the hostage. The engine mannequin already ships jog and dash animations that can carry the movement.

Sprint: [[Sprint 2 - Three scenes]] · Stream: fight
