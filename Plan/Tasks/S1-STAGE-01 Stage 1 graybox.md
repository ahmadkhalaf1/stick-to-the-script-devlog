---
sprint: 1
stream: stage
status: done
owner: lead
estimate: 3
depends: []
---

**Goal:** A film-set warehouse the first scene can be played on.

**You own:**
- `Content/Scenes/Stage01 (one owner)`
- `Content/Props/`
- `Content/Materials/`

**Done when:**
- [x] Floor, three walls, key lights (a closed room: four walls and a ceiling, a warm key, a cool rim and a soft fill)
- [x] Named actors exist: PlayerMark, PartnerMark_A, PartnerMark_B, Cam_Wide, Cam_Close, Cam_Low (found by tag; PartnerMark_C is there too)
- [x] It is the editor startup map and the game default map (ask the lead to set the config)

**Notes:** The names are a contract: code finds marks and cameras by them.

- **The level is generated.** `Tools/build_stage01.py` is its source: run it with the editor closed (the command is at the top of the file) and it rebuilds `Content/Scenes/Stage01` and the five materials in `Content/Materials`. Hand edits made in the editor are lost when it runs again, so change the script, or stop using the script once the art pass starts.
- The exposure is fixed by a post-process volume. Without it the engine brightens a closed room to an even grey and the pool of light is lost.
- No editor tool could create a level, and a copied engine map cannot be saved, which is why it is a script.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: stage
