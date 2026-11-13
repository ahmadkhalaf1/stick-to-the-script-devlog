---
sprint: 1
stream: fight
status: done
owner: lead
estimate: 5
depends:
  - "[[S1-FIGHT-01 Fighter move set]]"
  - "[[S1-CORE-02 Take director]]"
---

**Goal:** An opponent that walks to its mark, shows a tell, attacks on its beat and reacts when the player's move lands.

**You own:**
- `Source/StickToTheScript/Fighter/StuntPartner.*`

**Done when:**
- [x] Plays a whole four-beat scene with the director
- [x] An autoplay debug switch lets it run with no player input (`sts.Autoplay 1`)

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: fight

**Notes:** The partner does not walk to its mark yet: it is spawned on it. Its tell is the slowed wind-up of its strike, which is stretched to land just after the beat window closes. S2-FIGHT-02 makes the tells readable.
