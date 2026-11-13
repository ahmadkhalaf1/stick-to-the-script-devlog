---
sprint: 1
stream: ship
status: done
owner: lead
estimate: 2
depends: []
---

**Goal:** Proof, on day one, that the project packages and runs outside the editor.

**You own:**
- `Tools/package.ps1`

**Done when:**
- [x] Windows Development build made from the command line (`Tools/package.ps1`, editor closed, about 3 minutes)
- [x] It runs on the lead's machine (a clean take, the film and the next take, with no errors in its log)
- [x] Its size is in the dev log (629 MB before zipping)
- [x] The MCP and toolset plugins are confirmed absent from the package (the script checks every build)

**Notes:** Do this early. Packaging problems found on Sunday are fatal.

- One was found: the agent plugins pull in a plugin that logs an error at start-up, and any logged error fails the cook. The cook now runs with those plugins disabled.
- The package is in `Saved/Package/Windows`, which is not committed. Half of the 629 MB is the Development executable; a Shipping build would be much smaller, but the jam asks for Development.
- Still to do in S4: zip it as `TeamName_GameName.zip` and check it is under 1 GB, and try it on a second machine.

Sprint: [[Sprint 1 - One scene, start to finish]] · Stream: ship
