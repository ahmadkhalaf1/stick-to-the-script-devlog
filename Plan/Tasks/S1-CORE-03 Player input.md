---
sprint: 1
stream: core
status: done
owner: lead
estimate: 2
depends:
  - "[[S1-CORE-01 Shared types and events]]"
---

**Board tidy, Fri Nov 13:** in the game; the developer has played with it.

**Goal:** Block, duck, punch and kick on keyboard and gamepad, forwarded to the director.

**You own:**
- `Source/StickToTheScript/Core/StickPlayerController.*`
- `Content/Input/`

**Done when:**
- [ ] Both devices work in Play-In-Editor (keyboard verified; the gamepad is bound but untested, see Notes)
- [x] The bindings are listed in this note

**Bindings:**

| Move | Keyboard | Gamepad |
|---|---|---|
| Block | A or Left | B (right face button) |
| Duck | S or Down | A (bottom face button) |
| Punch | W or Up | X (left face button) |
| Kick | D or Right | Y (top face button) |

The console command `SubmitMove Punch` (or Block, Duck, Kick) does the same without a device.

**Notes:**
- The bindings are built in code in `AStickPlayerController::SetupInputComponent`, so `Content/Input/` stays empty and there is no binary input asset to keep in step with `EMove`.
- `AStickGameMode` (also in `Core/`) sets this controller, removes the default flying pawn (it would move on WASD) and spawns an `ATakeDirector` if the level has none. It is the project's default game mode in `Config/DefaultEngine.ini`.
- Verified in Play-In-Editor by pressing W, A, S and D through the editor's UI automation. Still to do by hand: play one take with a gamepad, and try the arrow keys.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: core
