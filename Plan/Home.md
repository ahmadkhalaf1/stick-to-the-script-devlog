# Stick to the Script — plan

**Deadline:** Sunday Nov 15, 5:59 PM PST. That is Monday Nov 16, 01:59 UTC (02:59 in UTC+1). Late entries are disqualified, so we aim to submit by Sunday 20:00 (UTC+1).

## The pitch

A playable movie. You are a stunt performer hired for a scene, and the script says you lose the fight. You do not stick to the script. Nobody tells you what to press: you walk in and win the scene your own way, while the director shouts that none of this was written. Then you watch the film you made: the Director's Cut, edited from what you really did. Going down the way the script wanted is a "Cut!", and every retake comes out of your pay.

## Where things are

- [[Board.base]]: every task with its owner and status, grouped by sprint, owner, stream or status.
- [[Team]]: who is on the team and which streams each person owns.
- [[Contracts]]: the shared types, events and names that let separate work fit together.
- [[CLAUDE]]: the team rules. They bind people and AI agents alike.
- [[DEVLOG]]: the dev log, which is also a prize entry.

## Phases

| Phase | Sprint | Ends (UTC+1) | True at the end |
|---|---|---|---|
| A. Prove it | [[Sprint 0 - Setup]], [[Sprint 1 - One scene, start to finish]] | Fri 12:00 | One scene plays end to end in a packaged build |
| B. Build it | [[Sprint 2 - Three scenes]] | Sat 12:00 | Three scenes, title to credits, with sound |
| C. Finish it | [[Sprint 3 - Feel and lock]] | Sun 12:00 | Tuned, under 1 GB, features frozen |
| D. Ship it | [[Sprint 4 - Ship]] | Sun 22:00 | Submitted |

Every sprint ends with an integration checkpoint: everything merges, the lead makes a packaged build, and new work waits until `main` is working.

## Direction change, Thu Nov 12

The fight is played from outside, with both fighters seen whole. Through the body cam the best moments happened out of frame. The body cam stays as a second angle (V key) and for point-of-view inserts in the final cut. The time saved goes to making hits felt: [[S1-FEEL-03 Impact pass]]. [[S2-CAM-03 Body cam look]] is now optional.

## Direction change 2, Thu Nov 12 evening

Cued beats and key prompts are gone. A scene is a free fight ([[S2-FIGHT-04 Free fight]]): the player moves, strikes, ducks, guards, jumps and throws things freely, and opponents think for themselves. The title became the joke. What ties it to the theme: the script card before each scene, the director who protests, and the film cut from the player's own fight. And the pay scores the film: a "box office" figure that rewards variety and pays nothing for the same move over and over ([[S2-CORE-05 Box office]]).

## If we fall behind

The plan holds about 140 hours of work before the freeze. It was sized for three to five people, and the run is now solo with agents, so expect to cut. Cut in this order, and decide at each checkpoint, not on Sunday:

1. [[S2-FIGHT-01 More ways to hit]] (keep four moves)
2. [[S2-STAGE-03 Stage 3 - the rooftop]] (ship two scenes)
3. [[S2-CORE-02 Director's notes]]
4. [[S2-CAM-03 Body cam look]] (keep the plain first-person view)
5. [[S2-STAGE-02 Stage 2 - the bar]] (ship one polished scene)

Never cut: the final cut replay, the early packaged build, the gameplay video, the dev log.

## How a task works

1. **Get it.** The lead sets `owner` on the task notes at the start of each sprint. To pick up an unassigned task mid-sprint, ask the lead.
2. **Check what it needs.** Its `depends` list must be done, or you build against [[Contracts]] until it is.
3. **Work in your own space.** One branch per task, and only the paths under "You own".
4. **Finish it.** Tick every "Done when" box, run the pre-merge check from [[CLAUDE]], merge, set `status: done`.

## Status values

`todo`, `doing`, `blocked` (say why in the note), `review`, `done`.

## Streams

| Stream | Covers | Owns |
|---|---|---|
| core | Take loop: the director, input, game flow | `Source/StickToTheScript/Core/`, `Content/Input/`, `Content/Scripts/` |
| fight | Fighters: moves, defence, stunt partners | `Source/StickToTheScript/Fighter/`, `Content/Characters/` |
| cam | Body cam, take recorder, final cut, bloopers | `Source/StickToTheScript/Cam/`, `Content/Camera/` |
| stage | Levels, lighting, props | `Content/Scenes/`, `Content/Props/`, `Content/Materials/` |
| feel | Sound, music, interface | `Source/StickToTheScript/UI/`, `Content/UI/`, `Content/Audio/` |
| ship | Repository, builds, store page, video, dev log | `Tools/`, `Docs/`, `DEVLOG.md` |
