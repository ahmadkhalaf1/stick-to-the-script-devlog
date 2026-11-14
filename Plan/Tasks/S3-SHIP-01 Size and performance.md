---
sprint: 3
stream: ship
status: doing
owner: lead
estimate: 3
depends:
  - "[[S2-SHIP-02 Integration checkpoint 2]]"
---

**Board tidy, Fri Nov 13:** the zip is 379 MB; 106 frames a second at 1080p on the developer's PC; no other PC tried.

**Goal:** The package is safely under the limit and runs smoothly.

**You own:**
- `Config/ (through the lead)`
- `Tools/`

**Done when:**
- [x] Zip under 1 GB, target under 500 MB (379 MB on Fri Nov 13)
- [ ] 60 fps at 1080p on a mid-range PC. Measured on Sat Nov 14 on the developer's PC (RTX 3070, Ryzen 5 5600X), 1080p in a window while it was being recorded: 96 frames a second on average over 256 seconds, 87 in the worst ten. No slower PC has been tried
- [ ] Unused content and plugins removed

Sprint: [[Sprint 3 - Feel and lock]] · Stream: ship
