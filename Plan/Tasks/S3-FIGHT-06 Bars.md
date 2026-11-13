---
sprint: 3
stream: fight
status: done
owner: lead
estimate: 4
depends:
  - "[[S3-FIGHT-05 More moves]]"
---

**Board tidy, Fri Nov 13:** every box ticked; not played by hand by the agent.

**Goal:** Jump under a bar, hang from it, and kick off it into whoever stands in front.

Asked for by the developer in the night of Thu Nov 12 to Fri Nov 13: "maybe a climb jump hit where character can use bars to jump and hang at it then jump kick the enemy".

**You own:**
- `Source/StickToTheScript/Fighter/SwingBar.*`
- `Source/StickToTheScript/Fighter/StickFighter.*`

**Done when:**
- [x] A scene file lists its bars (`Bars`: an `X` and a `Height` each), and the game hangs a pipe there, across the set
- [x] Jumping under a bar grabs it: the fighter hangs by both hands and sways
- [x] Kick or punch while hanging swings both feet forward into everyone ahead, lets go, and lands further on
- [x] Jump while hanging lets go. So does being hit, and so does hanging too long (in the code; only the kick was seen)
- [x] It has its own sounds (a clang on the grab) and pays at the box office as "off the bar" ($40 was seen paid; the sounds are unheard)
- [x] The test fighter uses a bar when one is in its way, so self-played runs show it (it does so now and then, not every fight)
- [x] Each scene has at least one bar, and the slate and the pause menu say how it works

**How it works:**
- `ASwingBar` is a pipe across the set, hung from above so nothing stands between it and the camera. The game mode spawns one for each entry of the scene's `Bars`.
- Only a jump the fighter chose can grab a bar: one thrown into the air by an uppercut does not.
- A hanging fighter is in the air, so only a shot or a strike from the air reaches it. It cannot stay: it lets go after a couple of seconds.
- The pose is made in code like the other moves: the shoulders are put an arm's length under the grip and the body turns about the bar.

**The rest of the night's order, decided Fri Nov 13 about 03:50, in case this note is all that is left of it:**
1. This task.
2. Better models for what is thrown and held, and three weapons that are not golden: a wooden bat, an iron bar, a hammer. Try building real meshes with the engine's Geometry Script from the stage scripts' Python; if that does not work headless, better materials and more pieces.
3. A small shop on the title menu to spend the banked money: those three weapons, bought for one scene each and lying at the player's feet when it starts, and hats or colours that stay bought. Ownership is kept where the progress is, in the settings file.
4. A long self-played run of the packaged game, store screenshots taken from the packaged game's window, package, dev log, push, and the PC to sleep.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight

**Where the night stood at 04:35 on Fri Nov 13:** items 1 to 3 of the order above are done, merged and pushed (`main` at `0296a1e`), and the packaged build in `Saved/Package/Windows` has all of them. Still to do, in order: (a) read the log of the 16-minute self-played run of the packaged game (`%TEMP%\sts-soak.log`; captures of its window are in the session's scratchpad, folder `soak`) for errors, stuck fights and frame rates; (b) pick five or six captures for the store page into `Docs/store`; (c) delete the `[StickToTheScript.Progress]` section from `Saved/Package/Windows/StickToTheScript/Saved/Config/Windows/GameUserSettings.ini`, which the self-played runs filled, so the developer starts clean; (d) a last dev log entry that sums up the night, commit, push; (e) put the PC to sleep, as the developer asked.
