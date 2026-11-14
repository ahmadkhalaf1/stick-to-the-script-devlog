---
sprint: 3
stream: ship
status: doing
owner: lead
estimate: 1
depends:
  - "[[S3-SHIP-02 Bug bash and feature freeze]]"
---

**Goal:** The editor opens without the Message Log window asking for an asset manager entry for `GameFeatureData`.

**You own:**
- `StickToTheScript.uproject` (with the lead's word)
- `Tools/package.ps1`

**Why it showed:** the `AllToolsets` bundle depends on every agent toolset. Two of them, `GameFeaturesToolset` and `ConversationToolset`, switch the `GameFeatures` plugin on, and that plugin reports an error at every start when the asset manager has no rule for its data. The rule it asks for cannot go into `Config/DefaultGame.ini`: the cook runs without the agent plugins, cannot load the class the rule names, and fails.

**What was done:** the project file names the toolsets one by one, without those two. `Tools/package.ps1` reads from the project file which plugins are editor-only, cooks without them and checks that none is in the package.

**Done when:**
- [x] The editor's log has no line about `GameFeatureData` and the plugin is not mounted
- [x] The agent tools still answer (a scene played and photographed through them)
- [x] `Tools/package.ps1` prints `PASS` and the packaged game plays a scene
- [ ] The developer has opened the editor and seen no Message Log window

Sprint: [[Sprint 3 - Feel and lock]] · Stream: ship
