# Contracts

What the streams agree on, so work done separately fits together at the checkpoint. The core owner maintains this note and the header it describes. To change a contract, ask the core owner; do not edit the header yourself.

**Status: built.** The code was created by [[S1-CORE-01 Shared types and events]] and this note matches it. If the two ever differ, the code is right: tell the core owner.

The include root is `Source/`, so every stream writes `#include "StickToTheScript/Core/TakeTypes.h"` and `#include "StickToTheScript/Core/TakeEvents.h"`.

## Types

In `Source/StickToTheScript/Core/TakeTypes.h`. All of them can be used in Blueprints.

| Name | What it is |
|---|---|
| `EMove` | None, Block, Duck, Punch, Kick, Throw, Shot, then the moves posed in code: Uppercut, Sweep, Roundhouse, Chop, SwingKick. New moves are appended, never reordered. `IsHeavyMove(Move)` says whether a blow has the body's weight behind it. |
| `FSceneScript` | A scene: `Name`, `Synopsis` (what the screenplay says happens), `Stage` (the level it is shot on), `Fee` and `CutCost`, `PlayerStart`, `ArenaMin` and `ArenaMax` (the floor fighters may walk on), `Opponents`, `Props`, `Captives` (places outside the arena where someone waits to be rescued), `Epilogue` (shown on the last card of the run when this is the last scene). Every scene is a free fight: nobody cues the player, and the take ends when one side is down. Scenes are written as `Content/Scripts/Scene*.json` and shot in file-name order. |
| `FOpponent` | One opponent: `Kind` (`EOpponentKind`: Thug, Rusher, Kicker, Heavy, Gunman), `Enter` (where it waits), `Spot` (where it comes to when it joins), `WakeDistance` (it joins when the player is this close to its spot, or when fewer than two others are fighting) and `Color`. |
| `FSceneBar` | A pipe overhead to hang from and kick off: `X` (where it crosses the set), `Height`, and `bOnAPost` for a set with no ceiling. A scene lists them in `Bars`; the game mode spawns an `ASwingBar` for each. |
| `FSceneProp` | Something to pick up and throw: `Kind` (`EPropKind`: Crate, Bottle, Melon, Cone, Pan, and the legendary Hammer, Bat and Fish, which are not placed by a scene), `Location` and `Height` above the floor. |
| `ECutReason` | KnockedOut: the player's fighter went down. It is the only way a take is cut. |
| `FTake` | A recorded take: `Number`, `bClean`, `Events`, `Frames` and `FighterColors` (one per fighter, so a replay shows the characters as they were played). |
| `FRecordedEvent` | One entry of `FTake::Events`: `Kind` (`ERecordedEventKind`: Hit, Blocked, Cut), `TimeSeconds`, and the fields its kind uses out of `Move`, `AttackerIndex`, `VictimIndex`, `CutReason`, `ImpactPoint` (where the strike landed) and `bKnockout`. The rest keep their defaults. |
| `FTakeFrame` | One entry of `FTake::Frames`: `TimeSeconds`, `Fighters` (one `FFighterPose` per fighter) and `Props` (one transform per loose prop, in the scene's order). |
| `FFighterPose` | `Joints`: one fighter's world positions, in the order `GetJointPositions()` returns them. `BrokenArms`: bit 0 is the left arm, bit 1 the right, and bit 2 says the fighter holds a gun. |

Rules the code cannot enforce:

- Times are seconds since the take started.
- Stunt partners are numbered 0, 1, 2 for `PartnerMark_A`, `_B`, `_C`.
- Fighters keep one order in every frame: the player first, then partners A, B, C. `AttackerIndex` and `VictimIndex` use that order.

## Events

On `UTakeEvents`, a world subsystem. Everything outside the core stream reacts to these and never calls the director directly.

| Event | Fired when | Typical listeners |
|---|---|---|
| `OnTakeStarted(int32 TakeNumber)` | a take begins | interface, recorder |
| `OnHit(AActor* Attacker, AActor* Victim, EMove Move)` | an attack lands | sound, recorder, final cut |
| `OnBlocked(AActor* Attacker, AActor* Defender)` | a strike lands on a raised guard | effects, sound, recorder |
| `OnCut(ECutReason Reason)` | the player's fighter went down | interface, sound, recorder |
| `OnTakeFinished(const FTake& Take)` | every opponent is down | final cut, game flow |
| `OnFinalCutStarted(const FTake& Take)` | the film of a clean take begins | interface, effects |
| `OnFinalCutFinished()` | the replay ended | game flow |
| `OnCard(const FText& Headline, const FText& Detail)` | a full-screen card goes up (title, scene, pay, results); an empty headline takes it down | interface |
| `OnPayChanged(int32 Pay)` | a take starts, with what the scene would pay if this take is the one | interface |
| `OnBoxOffice(const FText& What, int32 Dollars)` | something the player did in the shoot earned money at the box office, or earned $0 because it was seen too often | interface |
| `OnAction()` | a free fight goes live: the lead-in is over and everyone may move | controls, opponents, interface |
| `OnSwing(AActor* Attacker, EMove Move)` | a strike or a throw starts in a free fight; it may still miss | sound |
| `OnFighterDown(AActor* Fighter, bool bPlayer)` | a fighter is knocked out in a free fight | director |
| `OnOuttake(int32 TakeNumber, bool bCut)` | after the film, the end of a take that was cut is shown again: fired as it starts, and once more at its "Cut!" | interface, sound |
| `OnFinish(AActor* Attacker, int32 Beat)` | a blow of the big finish has been started: three on one opponent, counted from 0; its hit follows as `OnHit` a moment later | slow motion, the camera's cuts |

How to use them:

- Get the subsystem with `GetWorld()->GetSubsystem<UTakeEvents>()`.
- Listen with `Events->OnCut.AddDynamic(this, &UMyClass::HandleCut)`. The handler must be a `UFUNCTION()` with the parameters in the table. Blueprints can bind the same events.
- Fire with `Events->OnCut.Broadcast(Reason)`.
- Every event writes one line to the `LogTakeEvents` log category when it fires, so the Output Log shows the order of a take.

## Fighter

On `AStickFighter`, in `Source/StickToTheScript/Fighter/`.

- `PlayMove(EMove)` plays a move and returns to idle. An attack played this way hits nobody.
- `Attack(Move, Victim, bKnockout, SecondsUntilContact)` throws a strike that lands on `Victim` at its contact moment. The fighter fires `OnHit` then, and the victim reacts where it was struck. A heavy strike (Kick) that does not knock out breaks the nearer arm, which hangs loose until the next take.
- `GetLastImpactPoint()` is where the last strike landed on this fighter, and `IsDown()` says whether it is knocked out. Effects read these when `OnHit` or `OnBlocked` fires.
- Every fighter stands up, mends, gets its health back and returns to idle by itself on `OnTakeStarted`.
- Free fighting: `SetArena` switches it on. `Walk(Direction)`, `Jump()`, `Guard(Block or Duck)` held by calling it every frame, `Strike(Move, SecondsUntilContact)` and `GrabOrThrow()`. A strike has no chosen victim: at contact it lands on whoever of the other `Team` is within the reach of its animation (or of the kicking leg) plus a body's radius, inside about 70 degrees of straight ahead. The fighter steps into the strike to arrive touching. `SufferBlow` is the one way anything hurts a fighter: it fires `OnHit`, and `OnFighterDown` when health runs out.
- A punch misses a ducked head; a raised guard facing the attacker turns a hit into `OnBlocked`; the third quick blow in a row, and every kick, sends its victim flying.
- `SetPuppetPose(Pose)` turns a fighter into a puppet that shows exactly that recorded pose, and `EndPuppet()` gives it back. The final cut poses the real fighters this way, so the film shows the same characters. `MarkStruck(ImpactPoint, bKnockedOut)` lets effects ask a puppet about a replayed strike.

## Recording and the film

- `AFightRecorder` (`Cam/FightRecorder.*`) records every fighter on every tick and every event, in the fighter order above. It is not called `TakeRecorder` because the engine already has a header of that name.
- `AFinalCut` (`Cam/FinalCut.*`) plays a clean take back as an edit. It does not start by itself: after a wrap the flow shows the choice of cut and calls `Start(EFilmCut)`. `PlanClips` picks the stretches it keeps: the first blow, every knockout and thrown prop, then kicks and stopped blows up to a length. Blows in the stretches it leaves out are not landed again. `Skip()` ends it; the game flow calls that when the player presses punch.
- `AFilmScore` (`UI/FilmScore.*`) is the music: `UFilmMusic` synthesises it, and the actor sets its mood from the events (silent on cards, a pulse from `OnAction`, the full track from `OnFinalCutStarted`).
- It It fires `OnHit` and `OnBlocked` again from the recording, so anything that reacts to those events works in the film with no extra code. During the film the attacker and victim are the same fighter actors, posed as puppets.
- `GetJointPositions()` returns the 16 world positions the stick is drawn from, always in this order: pelvis, chest, neck, skull, then shoulder, elbow, hand for the left arm and the right arm, then hip, knee, foot for the left leg and the right leg. The recorder stores these, and the final cut draws its puppets from them. Block, duck, flinch and the knockout ragdoll are already in these positions, so a recording needs nothing else.

`ACaptive` (`Fighter/Captive.*`) is someone to rescue: on the player's team and outside the arena, so nothing can hit it; it cowers until `OnFighterDown` has fired for every opponent, then stands. The recorder keeps it after the opponents, so the film shows it.

`AStuntPartner` is an opponent. A Gunman keeps its distance, aims along a line the player can see, and fires where the line was held. It thinks for itself (`StartThinking`): it joins in the scene's order, waits on its side of the player, takes one of two turns to close in and strike, guards, and loses its turn when hit. The game mode spawns the player's fighter, every opponent, and the props (`AThrownProp`).

## Tags in levels

Code finds these actors by **actor tag**, not by label, because labels do not exist in a packaged game. Give each actor the tag and the same label: `PlayerMark`, `PartnerMark_A`, `PartnerMark_B`, `PartnerMark_C`, `Cam_Wide`, `Cam_Close`, `Cam_Low`.

A level with none of them still plays: the fight is laid out around the world origin and watched from the side.

## The run of the game

- `AGameFlow` (`Core/GameFlow.*`) is the only thing that starts takes. The director no longer starts by itself.
- It also keeps the box office (`FBoxOffice`): it listens to `OnHit` and `OnBlocked` during the shoot only, so the film does not pay twice, and fires `OnPayChanged` with the fee plus the box office so far. What a scene pays is the fee, less the retakes, plus the box office of the take that was the one.
- `UStickGameInstance` (`Core/StickGameInstance.*`) loads the scene files and carries the scene number and the money between levels. It also keeps what lasts between sessions, `ScenesWrapped` and `Banked`, in the player's settings file.
- The menus are the flow's too: the title menu, and the pause menu behind `TogglePause()`. A menu is an ordinary card; the row the cursor is on starts with `>`, and the interface draws that row in gold.

## Debug switches

- `sts.Autoplay 1` in the console: the scene plays itself. In a free fight a plain test fighter walks up to the nearest opponent and hits it; cards answer themselves.
- `SubmitMove Punch` (or Block, Duck, Kick) in the console plays one move.
- `bDemoAllMoves` on a placed `AStickFighter` cycles through every move forever.
- With autoplay on, the keys 1, 2 and 3 go straight to that scene. Tests use this; there is no editor tool that opens a level.
- F9 switches `sts.Autoplay` on and F10 switches it off while playing. V switches between the outside view and the body cam.
- `Log LogStickFighter Verbose` prints one line per landed blow: how far apart the bodies were, the reach, and how far the fist or foot stopped from the body. Use it when a hit looks wrong.

## Asset names

- Sounds are `Content/Audio/SFX_*`; music is `Content/Audio/MUS_*`.
- Scene scripts are `Content/Scripts/Script_Stage01`, `Script_Stage02`, `Script_Stage03`.
