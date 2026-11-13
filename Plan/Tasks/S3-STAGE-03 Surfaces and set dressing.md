---
sprint: 3
stream: stage
status: done
owner: lead
estimate: 4
depends:
  - "[[S3-STAGE-01 Stages 2 and 3 art pass]]"
---

**Goal:** The sets stop looking like grey boxes.

Asked for by the developer on Fri Nov 13, after playing the night's build: "in 2026 , and AI and claude , the game feel poor , what can we do?"

**You own:**
- `Tools/build_textures.mjs`, `Tools/build_materials.py`, `Tools/stage_kit.py`, `Tools/build_stage0*.py`
- `Content/Materials/`, `Content/Scenes/`

**Done when:**
- [x] Walls and floors have a surface: a texture with relief and its own roughness, not one flat colour
- [x] A stretched block does not stretch its texture
- [x] The warehouse has a building around it: columns, windows, goods
- [x] The bar and the rooftop have lamps that show
- [x] The stick figures have a material of their own
- [x] Nothing is downloaded: every texture is painted by a script
- [x] Frame rate measured in the packaged game with the extra lamps: median 106 in the warehouse (108 before), 117 in the bar, 111 on the rooftop, never under 103, at 1920 by 1080 on the developer's PC

**How it works:**
- `node Tools/build_textures.mjs` paints six surfaces into `Saved/Textures`: Concrete, Brick, Planks, Wallpaper, Roofing, Metal.
- `Tools/build_materials.py` brings them into `Content/Materials` and makes `M_Surface` (lays a texture by world position, three ways, one per axis), `M_Figure`, `M_Light` and `M_Sign`.
- In a stage script: `surface(name, kind, tint, tile)` for a textured material, `glow(name, r, g, b, strength)` for one that gives light, `crate(...)` for a framed crate, `sign(...)` for lettering.
- A material that exists is left alone. To change one, delete its `.uasset` and run the script again, then the stage scripts.
- Textures from somewhere else (the jam's partner tools, for one) go in the same way: a colour picture named `T_<Name>_C` with roughness in its alpha and a normal map named `T_<Name>_N`.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: stage
