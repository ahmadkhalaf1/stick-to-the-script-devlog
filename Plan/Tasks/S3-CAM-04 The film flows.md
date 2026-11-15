---
sprint: 3
stream: cam
status: doing
owner: lead
estimate: 2
depends:
  - "[[S3-FEEL-13 The film on a screen]]"
---

**Goal:** What the developer said on Sat Nov 14, late, of the film and of the gameplay video: "the scenes and how the video is made feels abit strange , foucs points and how it suddenly end and switch to new scene also looks strange , we need to consider this for reply and for youtube video later".

**You own:**
- `Source/StickToTheScript/Cam/FinalCut.*`, `Source/StickToTheScript/UI/StickHUD.cpp` (what the HUD lays over the film)
- `Tools/cut_video.mjs`

**What was wrong, seen in a film photographed five times a second:**
- A close shot looked at whoever was hit. The hero was cut off at its edge or not in it, and after a knockout the camera followed the body to the floor without him.
- A wide shot came up for a fifth of a second between two close ones.
- A thrown prop or a shot that put someone down got a close shot of that one alone, hit from nowhere.
- The low camera stood wherever the set's axis put it, and as often as not behind the opponent, whose back filled a third of the picture.
- Two stretches of the film were joined by a plain cut, so the fighters jumped across the set. The slate popped on, and the film cut straight to the results.

**What was done:**
- A close shot looks half way between the two in the blow, never much lower than the one still standing, and moves back for a pair standing apart. When the two are more than 2.9 m apart the wide camera takes the picture.
- No shot is shorter than 1.1 s: a return to the wide shot that would be shorter is dropped, and a stretch of the film opens and ends on the close shot it is nearest to.
- A throw or a shot is left to the wide camera, which shows where it came from.
- The low camera stands on the hero's side when he is in the blow, so it is the opponent who faces it.
- The picture dips to black where two stretches are joined, comes up out of the title card's black, and goes out to black under the slate, which comes on over 0.4 s. The results then come up out of that black.
- The gameplay video's pieces fade in and out, picture and sound, and the video opens and closes more slowly.

**Done when:**
- [x] Both fighters of a blow are in its close shot, and the hero's head stays in it after a knockout: photographed on the warehouse, a picture every fifth of a second
- [x] No shot of a fifth of a second: none in the 130 pictures of the warehouse's film
- [x] The joins dip, the slate comes on, the film goes out: photographed
- [x] The packaged build has it: made on Sun Nov 15 a little after midnight, and it played itself to the end card
- [ ] The gameplay video cut again with fades, from a new recording
- [ ] The developer has watched a film and says whether it still feels strange

Sprint: [[Sprint 3 - Feel and lock]] · Stream: cam
