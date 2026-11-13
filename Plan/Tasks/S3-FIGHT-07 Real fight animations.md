---
sprint: 3
stream: fight
status: done
owner: lead
estimate: 6
depends:
  - "[[S3-FIGHT-05 More moves]]"
  - "[[S3-FEEL-05 Stunt dummy look]]"
---

**Goal:** The moves are real, motion-captured animations instead of poses computed in code.

Asked for by the developer on Fri Nov 13, as the next thing that makes the game ready: "Real fight animations".

**You own:**
- `Source/StickToTheScript/Fighter/StickFighter.*`
- `Content/Characters/Fight/` (new: the converted clips)
- `Tools/build_animations.py` (new)

**What is there (Fri Nov 13, evening):**
- The engine ships no fight animation beyond the three punches and the charged attack the game already uses. Its templates were searched.
- Eleven clips from Mixamo (Adobe's free library; royalty-free inside a game; to be listed in the submission) are in `Saved/Incoming/Mixamo`, downloaded by the agent in the developer's browser, as FBX with the X Bot's skin, 30 frames a second: Fighting Idle, Jab Cross, Hook Punch, Uppercut, Mma Kick, Roundhouse Kick, Leg Sweep, Flying Kick, Hanging Idle, Head Hit, Stomach Hit.
- Nothing in the game uses them yet.

**Done when:**
- [x] The clips are in the project on the mannequin's skeleton, made by a script from the FBX files: `Tools/build_animations.py` makes `Content/Characters/Fight/A_*`, eleven of eleven (Fri Nov 13, 16:52). Those eleven were empty: every one held the standing pose. Converted again at about 17:00 with the missing step, and the script now fails if nothing in a clip moves
- [x] Each move plays its clip: kick (Mma Kick), uppercut, sweep (Leg Sweep), roundhouse, chop (Hook Punch stands in), the stance (Fighting Idle). Left as they were, on purpose: the punches (the engine's three are real animations already) and the two moves on a bar (the hands have to be exactly on it). Jab Cross, Flying Kick and Hanging Idle are converted and not used. Seen closely: the kick, the roundhouse, the hook, the stance. The sweep was caught only in its crouch, and the uppercut behind another fighter
- [x] A hit that does not knock out plays a hit reaction (Head Hit above the chest, Stomach Hit below), for as long as the fighter is stunned. Seen: an opponent bent back by the hook, and the player thrown back by a blow. A blow to the body was not caught in a picture
- [x] The blow still lands when the hand or foot is furthest out, as now: the game finds that moment in each clip (kick 0.67 s at 133 cm, roundhouse 1.10 s at 142 cm, sweep 1.03 s at 102 cm, hook 1.17 s at 82 cm, uppercut 0.47 s at 67 cm). In the log, five kicks stopped 12 to 34 cm from the body they hit
- [x] The film shows the same motion, since it replays joints: seen in the warehouse's film
- [x] A move whose clip did not convert keeps its computed pose: a missing clip is not in the fighter's table, and a clip thrown with an arm that is broken steps aside for the pose. Read in the code, not tried by taking a clip away
- [x] Mixamo is in the asset register, the credits card and the store page's list (`Docs/StorePage.md`; the text on the draft page itself is brought up to date with the final build)
- [x] The developer has played it (Fri Nov 13, in the packaged build, while trying the keys). No verdict yet on how the moves look or on their longer reach

**How it is meant to work:**
- The stunt dummy is drawn from the bones of the hidden mannequin (`BuildJoints`), so a clip played on that skeleton shows by itself. Today a technique is a pose laid over the joints (`StartPosed`, `PoseTechnique`); with a clip it becomes `Play(clip)` with the contact found the way `FindContactSeconds` already finds it for the punches.
- The risky step is fitting Mixamo's skeleton to the mannequin's (an IK Retargeter, driven from editor Python: import the FBX files, make an IK Rig for each skeleton, map the chains, batch retarget). If it will not work headless, the fallback is to do it once by hand in the editor's Retarget Animations window.
- Root motion is ignored, as for the punches now.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight

**Wiring plan (written Fri Nov 13, 16:55, before any of it was done):**
- Load `A_MmaKick`, `A_Uppercut`, `A_LegSweep`, `A_RoundhouseKick`, `A_HookPunch` (the chop), `A_HeadHit`, `A_StomachHit`, `A_FightingIdle_Anim` in the fighter's constructor. A clip that is missing leaves its move as it is today.
- In `StartPosed`: when the technique has a clip, play it like a punch is played (`Play`, `FindContactSeconds`, rate so that contact comes after the technique's wind-up) instead of setting `bKicking`. The technique's own numbers (damage, cone, through a guard, pop, stun) must still apply: `LandFreeStrike` looks the technique up by `bKicking` today and has to look it up by the move.
- The clips are not "in place": Mixamo moves the hips, not the root. Hold the figure over its actor by taking the hips' sideways drift out of every joint in `BuildJoints` while a clip plays.
- A blow that does not knock out plays `A_HeadHit` or `A_StomachHit`, by where it landed, for as long as the stun lasts.
- `A_FightingIdle_Anim` replaces the idle of a fighter that fights freely; the hands-up stance computed in code then steps aside.
- Hanging from a bar and the kick off it keep their computed poses: the hands have to be exactly on the bar.
- The engine's three punches stay: they are real animations already.

**What was done differently (Fri Nov 13, 17:15):**
- The hips' drift is not taken out. Measured before wiring: every clip ends where it started, and what moves the hips is the step into the strike (37 cm for the kick, 57 for the roundhouse). It is part of the move, and holding the hips still would slide the standing foot instead.
- So a move reaches as far as its clip does, which is further than the poses did: the kick 133 cm where the pose gave about 88, the roundhouse 142, the sweep 102, the hook 82 for the chop's 52, the uppercut 67 for 42. An opponent standing closer than that steps back as it strikes, as it already could. This changes how the fight plays and has not been felt by hand.
- A technique keeps its own clock (wind-up, recovery) and the clip is fitted to it: only the last of the wind-up is shown, at no more than twice its speed, and the way back is hurried.
