---
sprint: 1
stream: ship
status: done
owner: lead
estimate: 2
depends: []
---

**Board tidy, Fri Nov 13:** one developer and one agent in the end, so no second machine ever cloned it and nobody was added to the team note.

**Goal:** Teammates can clone, build and work without being able to break main by accident.

**You own:**
- `Repository settings`
- `Tools/`

**Done when:**
- [x] Private remote with Git LFS
- [x] main is protected: no force-push, no deletion (by a push guard on each machine, see Notes)
- [ ] A second machine clones, runs Tools/check.ps1 and sees the stick fighters
- [ ] Every teammate is in [[Team]] with a handle and their streams

**Notes:** Needs the lead: who is on the team, and whether merges to main need a pull request.

GitHub only protects branches of a private repository on a paid plan, so `main` is guarded by `Tools/hooks/pre-push` instead. `Tools/check.ps1` switches it on, and `Tools/test-push-guard.sh` tests it. It stops accidents, not someone who pushes with `--no-verify`. A paid plan would add protection on GitHub itself.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: ship
