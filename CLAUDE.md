# Stick to the Script — 2026 Epic AI MegaJam entry (Unreal Engine 5.8)

These rules bind every person and every AI agent working in this repository. Read them before changing anything.

Jam page: https://itch.io/jam/2026-epic-ai-megajam — Official Rules: https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z
Theme: **Director's Cut** ("your vision, your specific edits, your final cut").

## The game

**Stick to the Script** (`StickToTheScript.uproject`, C++ module `StickToTheScript`). A playable movie: you are a stunt performer hired for a scene, and the script says you lose. You ignore it. Each scene is a free fight watched from the side (a body cam is a second angle): you walk, punch, kick, duck, guard, jump and throw things, with no prompts but one: blows fill a bar, and when it is full the screen shows the key (Q) for the big finish, three blows cut across the film's close cameras that take everyone near apart. Going down is "Cut!" and a retake; putting every opponent down is a wrap, and the take replays as an edited stick-figure action scene (the Director's Cut), cut the way the player picks: an action cut, a slow-motion cut or one long take. The film opens on a title card, ends on a slate, and is followed by the outtakes: the last seconds of the takes that ended in "Cut!". A fighter is sixteen joints, taken from the bone positions of the hidden engine mannequin (`Content/Characters/Mannequins`) and reworked in code; what is seen is a skinned model (`Content/Characters/Dummy`) whose bones are put on those joints every frame.

## Start here

1. The repository root is an Obsidian vault. Read `Plan/Home.md`; `Plan/Board.base` lists every task with its owner and status.
2. Work only on a task whose `owner` is you (or the developer you are working for). The lead assigns owners; to take an unassigned task, ask the lead.
3. Stay inside the task's "You own" paths, and build against `Plan/Contracts.md`.
4. When every "Done when" box is ticked, run the pre-merge check below, merge, and set the task's `status: done`.

## Never break the base

- `main` must always compile, open in the editor and play Stage 1. If you break it, fixing it comes before anything else.
- One branch per task, named `<stream>/<task-id>-<slug>`, for example `cam/S1-CAM-02-take-recorder`. Pull `main` into your branch before merging. Merge at least once per sprint.
- **Pre-merge check, every time:** close the editor, run `Tools/check.ps1` (it must print `PASS`), open the editor, play Stage 1 once.
- Binary assets (`.uasset`, `.umap`) cannot be merged. Edit only assets inside your stream's folders. One person per level file. Save only the assets you changed; never use "Save All".
- Moving, renaming or deleting an asset that someone else may reference is done only by the lead, at an integration checkpoint.
- Some files have a single owner. Ask instead of editing them:
  - core owner: `Source/StickToTheScript/Core/TakeTypes.h`
  - lead: `StickToTheScript.uproject`, `Config/*.ini`, `*.Build.cs`, `*.Target.cs`, `.gitattributes`, `.gitignore`, `.mcp.json`, `CLAUDE.md`
- Never commit `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`. Never force-push or rewrite `main`.
- `Tools/check.ps1` also switches on a push guard (`Tools/hooks/pre-push`) that refuses a force-push to `main` or its deletion. GitHub does not enforce this for us, so never bypass the guard with `--no-verify`.
- Agents: one task at a time, on its own branch, merged before the next one starts. If a fix needs a file outside your task's paths, stop and ask the developer.

## Stream ownership

| Stream | Owns |
|---|---|
| core | `Source/StickToTheScript/Core/`, `Content/Input/`, `Content/Scripts/` |
| fight | `Source/StickToTheScript/Fighter/`, `Content/Characters/` |
| cam | `Source/StickToTheScript/Cam/`, `Content/Camera/` |
| stage | `Content/Scenes/`, `Content/Props/`, `Content/Materials/` |
| feel | `Source/StickToTheScript/UI/`, `Content/UI/`, `Content/Audio/` |
| ship | `Tools/`, `Docs/`, `DEVLOG.md` |

## Privacy

Keep personal and employer information out of the repository: no real names, personal or work email addresses, or company names in code, configs, assets, the plan or the dev log. Use handles. Submission details (names, Epic-account emails) are collected outside the repository. Check `git config user.email` before your first commit, because it is published with the history.

## Dev log (Glass Box prize)

`DEVLOG.md` is a prize entry, not a notes file. After every milestone, append a timestamped entry (local time from the system clock, never guessed): what the developer directed, what the agent did, the result, and what went wrong. Add a row to its asset and tool register for anything new that is not made by the team. Only record what actually happened. The file merges by union, so appended entries from different people never conflict; do not rewrite other people's entries.

## Hard constraints (missing one disqualifies or wastes the entry)

- **Deadline: Sun Nov 15 2026, 5:59 PM PST** = Mon Nov 16 01:59 UTC (02:59 in UTC+1). Late entries are disqualified. Plan to upload a working build a day early and replace it.
- Unreal Engine only for this team (a team cannot also enter UEFN). Max 5 humans, unlimited AI agents.
- Nothing built before Nov 12 may go in undeclared: pre-jam prompts, scripts and assets must be declared in the submission.
- Submit a **packaged** build (unpackaged projects are rejected): Windows 10/11 (or macOS/Android/iOS), Development configuration per the jam page.
- Zip named `TeamName_GameName.zip`, **under 1 GB** (larger only via butler). Uploaded to our own itch.io page, then submitted to the jam.
- Third-party assets (Fab etc.) are allowed only with a valid licence and must be listed in the submission. No one else's IP.
- Gameplay must go beyond Epic's starter templates.

## Submission checklist

- Team name; each member's name and Epic-account email
- Game name
- Link to a 30–60 second gameplay video
- List of third-party assets and of AI/other tools used (needed for partner prizes)
- Declaration of anything made before Nov 12
- Dev log link (optional; required for the Glass Box prize)

## Judging (each scored 1–5)

Ambition and innovation, theme, fun factor, gameplay, visuals, level design and progression, audio.

## Engine and tooling

- Unreal Engine 5.8.3, launcher build. The UE 5.9 MegaJam Preview is only needed for web builds.
- Package: `Tools/package.ps1` (editor closed). It makes a Windows Development build in `Saved/Package` and fails if an editor-only plugin got in.
- The dev log as one web page, pictures included: `node Tools/devlog_page.mjs`. It lands in `Saved/Release/devlog.html`.
- To look at the packaged game from a session, capture its window with the Windows `PrintWindow` call; the editor's screenshot tools only see the editor.
- The dev log and the sources are public at https://github.com/ahmadkhalaf1/stick-to-the-script-devlog: a copy without binary assets and without history. `Tools/publish_devlog.ps1` refreshes it from what is committed, and refuses if it finds an address, a user path or a key in the copy. Its front page is `Docs/PublicReadme.md`.
- The store page is a draft at https://the-fighter-ps.itch.io/stick-to-the-script (edit form: https://itch.io/game/edit/5353264). To put a new build on it from a browser session: click "Upload files" in the page, then run `Toolschoose_file.ps1 <zip>`, wait for "Success", tick Windows, delete the old file, save. Chrome only opens the file chooser for the tab it is showing, and a tab the agent opens is in the background: ask the developer to click that tab first. A zip with the old one's name takes its place in the list. Pictures need no chooser: click "Add screenshots" (the page then holds a hidden file input for pictures) and hand that input the files with the browser tool's own file upload, which takes up to 10 MB. Deleting a picture makes the page ask in a pop-up, and a pop-up in a background tab freezes the page for the agent until the developer answers it. Making it public and submitting it to the jam are the developer's decisions.
- The gameplay video: `Tools\record_run.ps1` lets the packaged game play itself for five minutes and records its window with ffmpeg and its sound through the game's own `-RecordSound` switch; `node Tools/cut_video.mjs` then puts the two in step and cuts under a minute into `Saved/Release`. The PC has to be free of other games and left alone while it records.
- Release zip: `Tools/release.ps1 -Team PageTwelve`, after packaging. It leaves out the game's `Saved` folder, so nobody's progress ships, and fails over 1 GB.
- Stages are generated: `Tools/build_stage01.py`, `Tools/build_stage02.py` and `Tools/build_stage03.py`, with shared helpers in `Tools/stage_kit.py`. The command is at the top of each file. Change the script, not the level.
- Surfaces are photographs: `node Tools/build_textures.mjs` paints a plain set, `node Tools/fetch_surfaces.mjs --go` writes Poly Haven's (CC0) over them and the night sky beside them (needs ffmpeg), then `Tools/build_materials.py` brings them in and makes `M_Surface`, `M_Sky`, `M_Figure`, `M_Light` and `M_Sign`. Run all three before the stage scripts. A new surface is a line in `fetch_surfaces.mjs` and a name in `SURFACES` in `build_materials.py`. A material that exists is left alone: to change one, delete its `.uasset` first, because clearing a material from a script crashes the editor.
- Recorded sound is generated too. `node Tools/build_sounds.mjs` lists what is missing and what it would cost; `--go` asks fal.ai for it (needs `FAL_KEY` and ffmpeg); `--hear` checks the spoken lines by speech-to-text. `Tools/build_audio.py` brings the WAV files into `Content/Audio`; its header says how they are named. A sound with no recording is made in code.
- What the director shouts is in each scene's file, under `"DirectorLines"`. To change a line, edit it there, delete its `Saved/Audio/V_Scene<n>_<line>.wav`, and run the two sound scripts.
- Fight animations are Mixamo clips fitted to the mannequin: `Tools/build_animations.py` imports the FBX files of `Saved/Incoming/Mixamo` and converts them into `Content/Characters/Fight/A_<Clip>`. The raw import and the rigs land in `Content/Characters/Fight/Mixamo`, which is not committed. The script fails if a converted clip does not move. A move gets its clip in the fighter's constructor (`TechniqueClips` in `Fighter/StickFighter.cpp`); one without a clip keeps its pose worked out in code.
- The fighters' body is Mixamo's X Bot model: `Tools/build_character.py` takes it from an FBX of `Saved/Incoming/Mixamo` into `Content/Characters/Dummy/SK_Dummy` and marks `M_Figure` for use on a skinned model. `Fighter/DummyBody.cpp` poses it on the joints. Set a model on a component with `SetSkinnedAssetAndUpdate`: writing it directly passes in the editor and fails the cook.
- The hero and each kind of opponent wear one of the cast: textured Mixamo characters, one FBX each in `Saved/Incoming/Mixamo/Characters` named by part (`Hero.fbx`, `Thug.fbx`, `Rusher.fbx`, `Kicker.fbx`, `Heavy.fbx`, `Gunman.fbx`, `Boss.fbx`; download from mixamo.com as FBX Binary, T-pose). `Tools/build_cast.py` brings them into `Content/Characters/Cast/<Part>` with their own materials and textures, and `AStickFighter::WearCast` puts one on a fighter at run time; a part with no model keeps the dummy. Any model with Mixamo's bone names fits, because `Fighter/DummyBody.cpp` places bones by name. A new model is a file and a run of the script, which leaves alone a part that is already in the project: delete its folder to bring it in afresh. `Config/DefaultGame.ini` has the folder cooked.
- The wardrobe's colour dyes the hero's clothes. `build_cast.py` makes his two opaque materials instances of `Content/Characters/Cast/M_CastCloth`, which swaps what is dark and without colour in his pictures for a tint; `HERO_CLOTH` in the script names, for each picture, the box of it that is dark and is not cloth, and how deep its cloth is dyed. Another hero needs his own rows there. To change `M_CastCloth`, delete it and the hero's folder. The first colour is no dye. A hat is one or two plain shapes (`Hats` in `Fighter/StickFighter.cpp`) and has to sit on top of the cap he is modelled in.
- Wounds are drawn on the model as it was posed (`UDummyBody::FindDrawnJoint`), not on the joints. A limb that comes off one of the cast is a second copy of the model with everything else closed down (`UDummyBody::ShowOnlyLimb`). In the editor, H breaks every fighter's arms, then takes their limbs off, for looking at them.
- Props are generated too: `Tools/build_props.py` makes every mesh in `Content/Props` and their one material. It needs `-EnablePlugins=GeometryScripting` on the command line; the command is at the top of the file. A new prop is a mesh there and a row in `Looks` in `Fighter/ThrownProp.cpp`.
- Most props are scanned models from Poly Haven (CC0): `node Tools/fetch_scans.mjs --go` downloads the ones listed in it into `Saved/Incoming/PolyHaven`, and `Tools/build_scans.py` (after `build_props.py`, same command line) imports them into `Content/Scans` and makes each prop of `PROPS_TO_MAKE` from the pieces named there, under the name the game loads. It also makes what only dresses a set, from `SET_PIECES`; a stage script puts one down with `dress()`. A new scan is a line in each of the two files, and a row in the dev log's register and the store page's list. What makes a prop a gun is a row in `Guns` in `Fighter/ThrownProp.cpp`. New goods go at the end of the shop's list: a look that was bought is remembered by its place in it.
- Scenes are text files, `Content/Scripts/Scene*.json`. A new fight is a new file; it needs no compile, and `Tools/check.ps1` tests every one. Opponents join two at a time in the order the file lists them. A `Boss` goes last: he comes on when the others are down, and the hero is put back to full health as he does.
- The first menu shows each scene as a picture card: `Tools/ui/Scene<n>.png` and `Scene<n>_Locked.png` (the same out of focus), brought into `Content/UI` by `Tools/build_ui.py`. A new scene needs its two pictures and a higher `SceneCardCount` in `UI/StickHUD.cpp`; without them it gets a plain card.
- A new header must not share its file name with any engine or plugin header: Unreal refuses to build (`TakeRecorder.h` was taken).
- A helper in an anonymous namespace needs a name no other source file in the module uses. Source files are compiled merged together, so two files that each define `Gravity` clash. `Tools/check.ps1` builds that way; a compile from the editor or an IDE may not, and will hide it.
- Compile: `Tools/check.ps1`. The editor must be closed; a new `UCLASS` or `UPROPERTY` needs the restart anyway.
- Launch the editor with `-ModelContextProtocolStartServer` so agents can drive it. The MCP and toolset plugins are enabled for the editor target only and must never ship. The project file names the toolsets one by one instead of `AllToolsets`: `GameFeaturesToolset` and `ConversationToolset` switch on the `GameFeatures` plugin, which opens the Message Log at every start, and the entry it asks for fails the cook.
- `.mcp.json` points agents at the editor's MCP server, `http://127.0.0.1:8000/mcp`. It only connects while the editor is open. If the connection shows as failed after the editor starts, only the developer can re-dial it; the server still answers JSON-RPC over plain HTTP at that URL (`list_toolsets`, `describe_toolset`, `call_tool`).
- The MCP server runs tool calls one at a time on the game thread: never issue overlapping `unreal-mcp` calls.
- `.uasset` / `.umap` files are binary: change them through the editor or MCP, never with text edits.
- `CaptureViewport` requires an `annotations` argument; pass `gridSpacing: 0` and `maxLabelDistance: 0` for a clean image.
- The editor does not tick actors while its window is in the background unless throttling is off; `Config/DefaultEditorSettings.ini` turns it off for this project.
