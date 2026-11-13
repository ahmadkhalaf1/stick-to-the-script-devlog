---
sprint: 4
stream: ship
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-SHIP-03 Integration checkpoint 3]]"
---

**Goal:** The exact zip that gets uploaded.

**You own:**
- `Tools/`

**Done when:**
- [x] Packaged, zipped as TeamName_GameName.zip: `PageTwelve_StickToTheScript.zip`, 379 MB, on Fri Nov 13. It is made again after every change to the game
- [x] The zip has no `Saved` folder: running the packaged game writes its progress there, and a judge must start with the scenes locked. `Tools/release.ps1` leaves it out and fails if it finds one in the zip
- [x] The Visual C++ runtime files are next to the game, so it starts on a PC that never had them installed. `Tools/package.ps1` copies them and fails if they are missing
- [ ] Run from the zip on a second machine

**How it works:** `Toolspackage.ps1`, then `Toolselease.ps1 -Team <TeamName>`. The zip lands in `SavedRelease`. It fails over 1 GB, which is the jam's limit without butler. A stand-in zip came to 360 MB, and the game ran from an extracted copy of it.

Sprint: [[Sprint 4 - Ship]] · Stream: ship
