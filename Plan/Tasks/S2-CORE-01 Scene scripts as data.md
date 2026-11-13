---
sprint: 2
stream: core
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-CORE-02 Take director]]"
---

**Board tidy, Fri Nov 13:** three scenes are text files, and a test reads every one.

**Goal:** Scenes are authored in data assets, so nobody needs C++ to write a fight.

**You own:**
- `Source/StickToTheScript/Core/SceneScript.*`
- `Content/Scripts/`

**Done when:**
- [x] The Stage 1 script lives in an asset (a text file, `Content/Scripts/Scene01.json`, not a binary asset)
- [x] Changing a beat needs no compile
- [ ] A scene can ask for something other than a fight move: at least "hold still" and one action on a prop (hold still is in scene 2; no prop action yet)

**How it works:**
- Scenes are `Content/Scripts/Scene*.json`, shot in the order of their file names. Each has a name, the stage it is shot on, a fee, what a cut costs, and its beats.
- Text files instead of data assets: they can be read, diffed and merged, and an agent can write one. They are packed into the build by a staging rule in `Config/DefaultGame.ini`.
- An automated test loads every scene file and checks each beat. A second test loads each file over a scene that already has content, because the JSON reader keeps old values for fields a beat leaves out. That bug floored an opponent four beats early in the first run of scene 2.
- With no scene files at all, the built-in six-beat scene still plays.

**Notes:** Asked for by the developer on Thu Nov 12: different scenarios, and actions that are not fighting. The beat system already allows it. A beat is a cue, a window and the moves that count, so the same four keys can mean what the cue names: kick a door open, duck under a swinging lamp, catch a thrown bottle. Pressing nothing is already a valid answer when a beat lists `None`, which gives "hold still for the close-up". Props that get kicked into someone are cheap, because physics does the work.

Sprint: [[Sprint 2 - Three scenes]] · Stream: core
