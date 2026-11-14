---
sprint: 3
stream: fight
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-06 Usual keys]]"
---

**Goal:** Any move can be asked for while other keys are down, and a move asked for a moment too early still comes.

Asked for by the developer in the night of Nov 13 to 14, in one message before going to sleep, with the order not to wait for a decision between steps. In the developer's words: "fix the keys combination , need to be smooth for example if i am kicking and clicking and jump to kick it should work without me letting go other keys etc."

**To find out first:** which combinations fail today and why. Suspects: a move asked for while another is still going is dropped, not kept for the moment it can start; a kick in the air needs the jump to have finished starting; a held guard or duck turns the next strike into another move.

**The plan:**
- Keep the last move asked for, for a fraction of a second, and start it the moment the fighter can act.
- Let a jump start out of the tail of a strike, and a strike out of a jump, without a key having to be let go.

**Done when:**
- [x] Jump then kick, kick then jump, and punch-kick-punch pressed fast all come out, by the log, with a direction held throughout
- [ ] The developer has tried it

**Found:** a key pressed while a move was going was dropped, and a kick locked the fighter for 0.45 s after it landed. A held guard or duck was not a cause: it only changes which strike comes.

**Done:** the controller keeps up to three presses in order (`AskedFor` in `Core/StickPlayerController`), each for 0.75 s, and does each the moment the fighter can act. A jump comes out of the tail of a strike. The lock after a kick is 0.3 s. Putting the guard up or ducking empties the line, so a defence is not kept waiting behind strikes; checked by the log with the guard key held.

**Left open:** a press older than 0.75 s is dropped. Holding a strike key does not repeat the strike.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: fight
