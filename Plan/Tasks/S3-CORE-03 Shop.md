---
sprint: 3
stream: core
status: done
owner: lead
estimate: 4
depends:
  - "[[S3-STAGE-02 Prop models]]"
---

**Goal:** The money a player banks buys something.

Asked for by the developer in the night of Thu Nov 12 to Fri Nov 13: "consider small in game shop to spend the money where u can buy customization for character or tools one time use to fight with like basebat or iron bar or hammer".

**You own:**
- `Source/StickToTheScript/Core/GameFlow.*`
- `Source/StickToTheScript/Core/StickGameInstance.*`

**Done when:**
- [x] The title menu has a Shop row, and the shop shows what is in the bank
- [x] Three weapons can be bought: a wooden bat, an iron bar, a hammer. One purchase is for one scene: the weapon lies at the player's feet at the start of every take of the next scene shot, and is used up when that scene wraps or the player goes back to the menu
- [x] Character items can be bought and stay bought: a halo, a crown and royal purple paint
- [x] What is owned is kept between sessions, next to the progress, in the settings file
- [x] Not enough money says so and takes nothing (the rule is tested; the message was not seen, because the editor's bank was full)
- [x] One automated test for the buying rule

**How it is meant to work:**
- The shop is a menu like the title and the pause menu: `ShowMenu` and `ChooseFromMenu` in `GameFlow.cpp`, a new step. W and S move, punch buys, a Back row leaves.
- Money is `UStickGameInstance::Banked`, saved by `Bank`. Spending needs a `Spend(Price)` that refuses when the bank is short.
- Prices to start from: bat $150, iron bar $300, hammer $500, hats $100 to $400. A wrapped scene banks about $700 to $1,700.
- The wardrobe (`AStickFighter::LookCount`, `LookName`) must skip what is not owned.

Sprint: [[Sprint 3 - Feel and lock]] · Stream: core
