---
sprint: 2
stream: core
status: done
owner: lead
estimate: 6
depends:
  - "[[S2-CORE-01 Scene scripts as data]]"
---

**Board tidy, Fri Nov 13:** in the scene files and on the cards.

**Goal:** The scenes are one short film with a reason for every fight, not three unrelated brawls.

**You own:**
- `Content/Scripts/` (the scenes and their cue lines)
- `Source/StickToTheScript/Core/`, `Source/StickToTheScript/Fighter/` (a gun, a hostage, a prop move)

**Done when:**
- [x] Each scene's card says what the script has happen, and the three read as one story: the gang has a witness's kid; the hero is beaten in the warehouse, knocked out in the bar, and too late on the roof
- [ ] ~~The cue lines of a scene tell its story when read in order~~ (a free fight has no cue lines; the cards carry the story)
- [x] Opponents with guns: a red line shows the aim, follows the player, then holds still for a moment before the shot. Ducking under it, stepping out of it or jumping over it makes the shot miss. (The gun is not kicked away: the gunman is knocked out like anyone else.)
- [x] One prop is used in a fight (crates and bottles are thrown)
- [x] The last scene ends on a beat that is not a blow: the kid cowers on the helipad through the fight and stands up when the last opponent falls. The last card says how the film ends now
- [ ] A person has played the rooftop: whether the aim line can be read and ducked in time is unknown

**Built Thu Nov 12.** Three scenes, on the developer's question "how many scene should we make?": three acts, three sets, one new threat each. The third is the rooftop after all, not a back room: an outdoor night set looks least like the other two.

**Notes:** Asked for by the developer on Thu Nov 12: scenes need to be logical and fun and have a meaning or story, like a hostage or child rescue, with fights using guns, hands or things in the set, so it feels like a scene from a movie.

A proposal that fits three stages:

1. **The warehouse.** A lookout stands between you and the only lead. One on one. He talks when he goes down.
2. **The bar.** Three of them know where the hostage is and will not say. One against three, with a bottle and a stool.
3. **The back room** (replaces the rooftop). The boss has the hostage and a gun. Duck the shot, kick the gun away, take him down, then untie the hostage: the last beat is a held key, not a punch.

Keep it to cue lines, one gun, one or two props and one tied-up figure. No dialogue system, no cutscenes: the Director's Cut is the cutscene.

Sprint: [[Sprint 2 - Three scenes]] · Stream: core
