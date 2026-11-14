---
sprint: 3
stream: feel
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-08 Shop and wardrobe]]"
---

**Goal:** The menus, the shop and the wardrobe can be worked with the mouse alone.

Asked for by the developer in the night of Nov 13 to 14, in one message before going to sleep, with the order not to wait for a decision between steps. In the developer's words: "allow mouse to control the menu and game shop and customization etc."

**What is there now:** W and S move a cursor and the left click chooses, wherever the pointer is. The pointer itself does nothing, and in the game it is captured and hidden.

**The plan:**
- In a menu the pointer shows, the row under it becomes the chosen row, and a click on a row takes it.
- In the wardrobe a click on a swatch or a pip picks that colour, hat or build.
- The keys go on working as they do.

**Done when:**
- [x] A row is chosen by pointing at it and taken by clicking it: seen in the pause menu and the title menu, with the real pointer (03:54). The shop and the choice of cut draw their rows the same way and were not tried
- [x] A colour, a hat and a build are picked by clicking them: a colour was, and a click beside the panel went on to the scene. A hat and a build were not tried
- [ ] In a take the pointer is hidden again and the click punches. In the code; the tests click through the editor, which does not show it
- [x] The mouse works in the packaged game too: on Nov 14 the real pointer lit the title menu's rows one after another, and a click on the first scene opened the wardrobe
- [x] The shop is bought from with the mouse: tried in the editor on Nov 14 for the pistol and the wooden bat
- [ ] The developer has tried it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
