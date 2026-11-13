---
sprint: 2
stream: core
status: done
owner: lead
estimate: 4
depends:
  - "[[S1-SHIP-03 Integration checkpoint 1]]"
---

**Board tidy, Fri Nov 13:** title, wardrobe, scenes, film, results, credits, pause and shop are in.

**Goal:** A full run without the editor: title, three scenes, results, credits.

**You own:**
- `Source/StickToTheScript/Core/StickGameMode.*`

**Done when:**
- [x] Title → Stage 1 → 2 → 3 → credits (the run is title, three scenes, results; the credits are a card on the title menu, not after the last scene)
- [x] Retry and quit work (the results card goes back to the menu; Quit is on the title and on the pause menu. Seen in the editor from the pause menu only)
- [x] A scene-select with locks, and progress kept between sessions
- [x] Results show takes used (the results card has a line such as "Takes: scene 1 in 2, scene 2 in 1, scene 3 in 5". Tested as text; not seen on screen, because the test fighter cannot finish the rooftop)
- [x] The paycheck: each scene has a fee, every "Cut!" takes a bite out of it, and the results show what the player earned

**How it works:**
- `AGameFlow` runs the game: title card, scene card, the shoot, the film, the pay card, then the next scene's level or the results. `UStickGameInstance` carries the scene number and the money from one level to the next.
- Every move key goes to the flow first: it answers a card that is waiting, or passes the move to the director.
- Pressing Play in the editor on any stage starts at that stage's scene.
- **Asked for by the developer on Thu Nov 12:** passing a scene gives money and unlocks the next. The title is a menu: the scenes, credits, quit. A scene is locked until the one before it is wrapped. What is kept between sessions (how many scenes are open, and all the money ever made) is in the player's settings file, section `StickToTheScript.Progress` of `GameUserSettings.ini`; deleting that section starts over. With self-play on nothing is locked.
- P, Esc or the gamepad's start button pauses a take or its film: resume, back to the menu, quit. Every key still answers while paused, and the controller keeps them from the fighter.

**Notes:** The framing, decided Thu Nov 12: the player is a stunt performer hired for the scene. Pay is the score. Keep it to one number per scene and a total.

Sprint: [[Sprint 2 - Three scenes]] · Stream: core
