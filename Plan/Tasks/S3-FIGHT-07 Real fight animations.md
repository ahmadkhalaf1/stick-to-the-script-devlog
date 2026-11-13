---
sprint: 3
stream: fight
status: doing
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
- [ ] The clips are in the project on the mannequin's skeleton, made by a script from the FBX files
- [ ] Each move plays its clip: punch (Jab Cross), kick (Mma Kick), uppercut, sweep (Leg Sweep), roundhouse, chop (Hook Punch stands in), the kick off the bar (Flying Kick), hanging (Hanging Idle), the stance (Fighting Idle)
- [ ] A hit that does not knock out plays a hit reaction (Head Hit, Stomach Hit)
- [ ] The blow still lands when the hand or foot is furthest out, as now
- [ ] The film shows the same motion, since it replays joints
- [ ] A move whose clip did not convert keeps its computed pose
- [ ] Mixamo is in the asset register, the credits card and the store page's list
- [ ] The developer has played it

**How it is meant to work:**
- The stunt dummy is drawn from the bones of the hidden mannequin (`BuildJoints`), so a clip played on that skeleton shows by itself. Today a technique is a pose laid over the joints (`StartPosed`, `PoseTechnique`); with a clip it becomes `Play(clip)` with the contact found the way `FindContactSeconds` already finds it for the punches.
- The risky step is fitting Mixamo's skeleton to the mannequin's (an IK Retargeter, driven from editor Python: import the FBX files, make an IK Rig for each skeleton, map the chains, batch retarget). If it will not work headless, the fallback is to do it once by hand in the editor's Retarget Animations window.
- Root motion is ignored, as for the punches now.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
