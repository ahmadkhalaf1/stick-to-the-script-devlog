---
sprint: 3
stream: feel
status: doing
owner: lead
estimate: 1
depends:
  - "[[S3-FEEL-12 Health bars and a scene map]]"
---

**Goal:** What the developer asked for on Sat Nov 14, having played the new build: "while watching the reply there should be some indicator that shows this is a reply or the movie we recorded earlier". Asked what the agent would suggest, the developer answered first: "i suggest showing a screen at the end where we show a small thumbnail with play button we zoom inside the screen make it full screen and run the video inside it".

**You own:**
- `Source/StickToTheScript/UI/StickHUD.*`
- `Source/StickToTheScript/Cam/FinalCut.*` (how far the film has run, and a longer title card), `Tools/cut_video.mjs` (the same number)

**What it is:**
- The card that offers the cut shows the film waiting: a small screen with the scene's picture and a play button, over the three cuts.
- Choosing a cut opens that screen out, in 0.6 s, to the whole picture between the film's bars. It fades into the title card, and the film begins. The title card is 0.6 s longer for it, so its words are on for as long as before.
- While the film or an outtake runs, there is a play mark by its title and a gold line along the bottom bar that fills as it runs. This part was the agent's suggestion.
- The small screen's picture is the take just shot: the screen as it was 0.22 s after the last opponent went down, by the game's slowed clock, read back before anything of the HUD is drawn on it. The developer asked for that ("Use my take"). If it cannot be read, the scene's card from the first menu is shown.

**Done when:**
- [x] The small screen is on the card, and opens out into the film: photographed about every 0.15 s, on the warehouse and the bar
- [x] The play mark and the line are on the film
- [x] The titles do not show round the small screen while it opens (they did at first)
- [x] The small screen shows the take just shot: seen in the editor, and the packaged game's log says it read one, 1280 by 720
- [ ] The video cutter still finds the film in a recording
- [x] The packaged build has it: made on Sat Nov 14 at 22:54, and its first film played to its slate with no error
- [ ] The developer has seen it

Sprint: [[Sprint 3 - Feel and lock]] · Stream: feel
