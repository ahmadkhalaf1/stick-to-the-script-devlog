---
sprint: 2
stream: fight
status: done
owner: lead
estimate: 5
depends:
  - "[[S1-FIGHT-02 Block and duck]]"
---

**Board tidy, Fri Nov 13:** done as [[S2-FIGHT-04 Free fight]] and [[S3-FIGHT-05 More moves]].

**Goal:** Enough distinct moves that two players' final cuts look different.

**You own:**
- `Source/StickToTheScript/Fighter/`
- `Content/Characters/`

**Done when:**
- [ ] At least six distinct player moves
- [x] One prop hit if time allows (thrown crates and bottles, in [[S2-FIGHT-04 Free fight]])
- [x] A kick that is a kick, not the heavy punch that stands in for it (in [[S2-FIGHT-04 Free fight]])
- [ ] A hit staggers the body with physics, not only a flinch on the joints
- [ ] Something in the set breaks or gets knocked over in a fight

- [ ] Moves blend into each other instead of snapping, and the player steps towards the next opponent instead of turning on the spot
- [x] Opponents waiting their turn move: they circle, close in or react, so three opponents do not look like three statues (in [[S2-FIGHT-04 Free fight]])

**Asked for by the developer on Thu Nov 12, in two messages:** smoother and more realistic fights against several opponents, with better transitions between enemies, movement speed and responsiveness; and better fight animation, physics and effects. Ideas, cheapest first: a kick posed with the same leg solver the duck uses; a part of the body going limp for a moment when struck (the mannequin's physics asset is already used for knockouts); bottles and stools that fly when someone lands on them; dust and a flash on heavy hits.

Sprint: [[Sprint 2 - Three scenes]] · Stream: fight
