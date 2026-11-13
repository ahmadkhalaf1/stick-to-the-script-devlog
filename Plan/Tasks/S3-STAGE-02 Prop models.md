---
sprint: 3
stream: stage
status: doing
owner: lead
estimate: 4
depends: []
---

**Goal:** What is thrown and held looks like the thing it is, not like a cube and a cylinder.

Asked for by the developer in the night of Thu Nov 12 to Fri Nov 13: "use better assets and real graphic for the throwable or holdabel".

**You own:**
- `Tools/build_props.py`
- `Content/Props/`
- `Source/StickToTheScript/Fighter/ThrownProp.*`

**Done when:**
- [x] One script builds every prop as a real mesh: turned shapes for the bottle, bat, cone, pan and melon; built-up ones for the crate, hammer and fish
- [x] Wood, metal and glass look different: a material with roughness and metal, not one flat colour (each vertex carries its colour and how much of a metal it is, so a prop is one mesh with one material)
- [x] Three weapons that are not golden: a wooden bat, an iron bar, a hammer. They are held and swung, hurt more than a fist, and break after some blows (built and in the rules; nothing puts one on a set until the shop does)
- [x] The game uses the meshes, in play and in the film (seen in play; the film shows the same actors)
- [ ] Seen in a frame, each one (seen: crate, bottle, melon, pan, wooden bat. Not yet: cone, the golden three, the iron bar, the sledgehammer)

**How it is meant to work:**
- Nothing is downloaded: there was nobody awake to approve a download or an account. The meshes are generated, like the stages, so they stay "made by the team" in the asset register.
- First choice: the engine's Geometry Script from editor Python (`unreal.GeometryScriptLibrary_MeshPrimitiveFunctions`: revolve a profile, append boxes and cylinders, save as a static mesh in `Content/Props`). The plugin is for the editor only and must not ship.
- If Geometry Script will not run headless: keep the engine shapes, add more pieces to each prop, and give them the new material.
- In `ThrownProp.cpp` the table `Looks` says what each kind is. A prop that is swung instead of thrown is what a legendary one already is; the shop's weapons are the same without the one-blow knockout.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: stage
