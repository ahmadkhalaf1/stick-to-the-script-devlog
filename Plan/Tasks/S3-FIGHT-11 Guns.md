---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 6
depends:
  - "[[S3-FIGHT-10 A real body]]"
  - "[[S3-STAGE-04 Props that behave]]"
---

**Goal:** Guns on top of the fist fighting: picked up from the set, fired with the punch, brutal when they hit.

Asked for by the developer in the night of Nov 13 to 14. First as a question: what if the game became gun fights, with a shotgun, limbs shot off, weapons picked up from the ground, ricochets. The agent answered that replacing the fist fighting was three to five days and adding guns on top was five to seven hours. The developer: "Yes, add guns". Then, on the models: "use real guns models assets no stupid graphic", and on the engine's own: "i want realistic military guns no kids guns".

**You own:**
- `Source/StickToTheScript/Fighter/ThrownProp.*`, `StickFighter.*`, `DummyBody.*` (firing, limbs)
- `Source/StickToTheScript/UI/ImpactEffects.*`, `StickHUD.cpp` (flash, lines, ammo)
- `Source/StickToTheScript/Cam/FightRecorder.*`, `FinalCut.cpp` (shots in the film)
- `Source/StickToTheScript/Core/TakeTypes.h`, `StickGameMode.cpp`, `StickGameInstance.cpp`, `GameFlow.cpp`, `StickPlayerController.cpp`
- `Content/Weapons/` (new), `Content/Props/SM_Pistol, SM_Rifle, SM_Launcher`, `Tools/build_props.py`, `Content/Scripts/Scene*.json`

**How it works:**
- A gun is a prop like a crate: it lies on the set, drops in as the take is called, and is picked up with the grab key. With one in the hands the punch fires it, and goes on firing while it is held. Empty, it is thrown like anything else. A kick with a gun in the hands is still a kick.
- Three of them. The pistol: 8 rounds. The rifle: 30, emptied in three seconds if the button is held. The blaster: 4 shots of seven pellets each, a spread that throws whoever it hits across the set.
- A shot is a straight line. It stops in the first opponent it passes through, or comes off an end wall of the set once and tries again with half its sting. It is aimed like a throw, at the nearest opponent roughly ahead, and tilted to pass through that one's chest.
- A gun takes a limb off whoever it puts down, and the blaster off anyone it catches squarely: a forearm, or on a body that is down a shin. The model closes to a stump with a wound on it, and the limb flies off and lies where it lands. Someone who loses a forearm and stays up fights on with the other arm.
- The film has all of it: a shot is recorded whether it hits or not and fired again in the replay, and the lost limbs ride in the same byte of the recorded pose as the broken arms.
- Where they come from: a pistol by the player's mark in the warehouse, the blaster in the bar, the rifle on the rooftop; the gunman's pistol falls where he does; and the shop sells all three.
- On screen: the gun's name and its rounds under the health bar, gold, red when it is empty.

**The models:** the engine's own (`Content/Weapons`, a copy of its `Templates/TemplateResources/Standard/Weapons`), turned and re-centred into `Content/Props` by `Tools/build_props.py`. The first ones were boxes put together by the agent, which the developer rejected; these are white and look like toys, which the developer rejected too. Realistic ones are the next step.

**Done when:**
- [x] The pistol, the rifle and the blaster are picked up, fired and counted down, in the editor on self-play, one in each scene
- [x] Shots land and put opponents down: by the log, and seen
- [x] A limb comes off: five in one bar fight by the log, and a blue opponent in the film with a stump for a forearm
- [x] The film fires the shots again, with the flash and the blood
- [x] The packaged build has them: eleven shots in a minute of self-play, nothing missing
- [x] Realistic military models, as the developer asked: a service pistol and a bolt-action rifle, scanned, from Poly Haven. The rifle is now what its model is, five slow heavy rounds. The blaster is off the sets and out of the shop: no scan of a shotgun was found
- [x] The shop sells them and the next scene has what was bought. Tried on Nov 14 in the editor: the pistol bought with a click, $350 off the bank, "waiting for your next scene" on its row, the scene card saying it is at your feet, and it was there and picked up with 8 rounds
- [x] The gunman's pistol falls where he does and can be picked up. Seen on Nov 14 on the rooftop: one rifle shot put him down, the log has "SM_Pistol falls from the hand of StuntPartner_1 at 882, 45", it lay there, the key strip offered "Pick up" beside it, and it was picked up with 8 rounds and fired
- [x] A ricochet has been seen. On Nov 14, fired at the rooftop's left end wall from a metre and a half: the line stops at the wall in sparks and a thinner one comes back over the shooter's shoulder. The log has the shot coming off the wall at x -500. It shows for 0.07 s, so it took fourteen pictures in a quarter of a second to catch
- [x] The rifle bought in the shop ($600) is at the feet in the next scene, shouldered, with 5 rounds: seen on Nov 14
- [ ] The developer has played it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
