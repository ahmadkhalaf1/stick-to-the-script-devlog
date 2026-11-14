---
sprint: 3
stream: core
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FIGHT-11 Guns]]"
  - "[[S3-CORE-01 Tuning and playtests]]"
---

**Goal:** A fight is mostly hands and feet, with a gun now and then, and the opponents get their blows in. In play and in the video.

Reported by the developer on Sat Nov 14, watching the game play itself for the video: "i dont see them hitting back , and i am sure it wont show up in the vide", and "the video and play uses guns more than fist , it should balance".

**Measured before:** five takes of the game playing itself, all wrapped: 42 shots that hit against 60 blows with hands and feet, nothing blocked, and 2 blows taken in five takes.

**Why:** every scene had a gun lying at the player's starting mark, and the game's own player picked it up and emptied it before anyone reached it. Then it struck without a pause, and every blow that lands costs an opponent its turn, so nobody ever finished a strike at it.

**What changed:**
- The guns are not at the mark any more (`Content/Scripts/Scene*.json`): the warehouse's pistol lies past the third opponent, the bar's at the far right end, the rooftop's rifle between the two gunmen. A fight opens with hands and feet for a person too. The first gun on the rooftop is now the one the first gunman drops.
- The game's own player (`PlayItself` in `Core/StickPlayerController.cpp`) fires a gun only after five strikes of its own in a take, and at most three times a take.
- After every five strikes it stands still for 1.9 s, as someone out of breath would. What comes at it then it blocks half the times and takes the other half.

**Measured after:** the three scenes once each in the editor, six takes: 3 shots that hit, 93 blows with hands and feet, 7 thrown props, 9 blows blocked, 39 taken. The warehouse and the rooftop wrapped on the first take; the bar took four, the three lost ones with 7 or 8 blows taken each.

**Tuned the same afternoon:** a second run lost the bar four times running. The rest is now after every six strikes and 1.6 s long, and seven blows in ten are blocked. That fired no shot at all in three scenes, because the guns are no longer where the player starts, so it now also walks to a loaded gun lying within 3.8 m once it may shoot. Measured last: four takes, three wraps and one "Cut!" on the rooftop: 12 shots that hit, 70 blows with hands and feet, 8 thrown, 2 blocked, 18 taken.

**Done when:**
- [x] More hands and feet than shots, and blows taken and blocked in every take: by the log, in all three scenes
- [x] The bar is not lost three times running: won on the first take in the last two runs
- [x] The packaged build and the video have it: the recorded run of Sat Nov 14, 18:15, counts 87 blows with hands and feet, 11 things thrown and 9 shots that hit over four takes, and the video is cut from it
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: core
