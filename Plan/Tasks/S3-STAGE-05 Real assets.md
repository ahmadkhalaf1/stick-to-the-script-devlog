---
sprint: 3
stream: stage
status: doing
owner: lead
estimate: 8
depends:
  - "[[S3-FIGHT-11 Guns]]"
  - "[[S3-STAGE-04 Props that behave]]"
---

**Goal:** What the player picks up, buys and fights among is real: scanned, photographed things, not shapes put together in code.

Asked for by the developer in the night of Nov 13 to 14, in three messages: "i want realistic military guns no kids guns"; "also try to use realistic real life assets for throwable items and shop items"; "also try to fix the style and graphic of the scene to make it more modern and 2026 like. but always keep in mind the game rules and make it fit for the competition".

**The developer's standing orders for this night** (given before going to sleep): keep working until the usage limit is nearly reached, never into extra credit; do not wait for a decision between steps, take the next one; when the limit is reached, commit and push, leave a summary, and put the PC to sleep.

**Where the assets come from:** Poly Haven (polyhaven.com). Every model and texture there is CC0, public domain, with no account needed. That fits the jam's rule on third-party assets as long as each is listed in the submission. `node Tools/fetch_scans.mjs --go` downloads them into `Saved/Incoming/PolyHaven/<id>`, outside the repository.

**You own:**
- `Tools/fetch_scans.mjs`, `Tools/build_scans.py` (new), `Tools/build_props.py`
- `Content/Scans/` (new: what was imported), `Content/Props/`, `Content/Materials/M_Scan`
- the prop table in `Source/StickToTheScript/Fighter/ThrownProp.cpp`, the scenes' prop lists, the shop's list

**What replaces what:**

| In the game | Poly Haven model | Note |
|---|---|---|
| Pistol | `service_pistol` | Only the assembled pistol "a": frame, slide, hammer, trigger. The file also holds a second pistol and loose magazines |
| Rifle | `bolt_action_rifle_7_62` | Body, bolt "a", trigger, scope and wrap. It is a bolt-action rifle, so the game's rifle becomes slow and heavy: few rounds, a second between shots, one shot puts most opponents down |
| Blaster (pellet spread) | none found | Poly Haven has no shotgun. Taken off the sets and out of the shop until a realistic model is found; its code stays |
| Wooden bat | `baseball_bat` | |
| Iron bar | `crowbar_01` | Small as scanned (56 cm): scaled up |
| Sledgehammer | `sledgehammer_01` | |
| Golden hammer, bat, fish | `ornate_war_hammer`, `baseball_bat`, `antique_katana_01` | In the props' own gold. The fish becomes a katana |
| Crate | `wooden_crate_02` | 116 cm long as scanned: scaled to be carried overhead |
| Pan | `brass_pan_01` | |
| Cone | `WetFloorSign_01` | Poly Haven has no traffic cone |
| Melon | `bar_chair_round_01` | No melon either. A bar stool is what gets thrown in a bar fight |
| Bottle | one bottle of `wine_bottles_01` | |
| (new) | `stick_grenade` | Fetched; not in the game yet |

**For the sets:** `node Tools/fetch_surfaces.mjs --go` fetches nine photographed surfaces and writes them over the painted textures under the same names, so the sets' material and layouts did not change; a tint now multiplies a photograph, and the stage scripts' tints were set again. `Tools/build_scans.py` makes thirty set pieces (`SET_PIECES`) that a stage script puts down with `dress()`. The roof has a photographed night sky (`M_Sky`, `sky()` in `Tools/stage_kit.py`).

**Decided on the way:** the sky is one with nothing but sky in it. The first one tried was a city skyline: it brought a famous real tower, lit company signs and people into the picture. The city behind the roof stays the team's own box towers, with their light turned down.

**Done when:**
- [x] The two guns are the scanned ones, in play: the pistol fired in the warehouse and the rifle on the rooftop, seen. Not looked at in the film yet
- [x] Every throwable and every shop weapon is a scanned model: all thirteen seen side by side in the warehouse. The golden three keep their scanned shape and wear plain gold; the bottle wears plain green glass, because its scanned glass is see-through and has no pictures
- [x] The sets use scanned surfaces and have scanned dressing: all three played by themselves in the editor and were photographed
- [x] The packaged build has all of it and stays under 1 GB: Packaged afterwards: 825 MB as a folder, where it was 663 MB. The packaged game then played itself for three and a half minutes, through all three scenes and their films, at a median of 96, 106 and 104 frames a second and never under 89. The upload zip made from it is 547 MB.
- [x] Every model and texture is in the dev log's asset register and in the store page's list
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: stage
