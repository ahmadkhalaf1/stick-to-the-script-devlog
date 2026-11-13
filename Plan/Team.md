# Team

Use handles here, not real names (see the privacy rule in [[CLAUDE]]). The jam allows up to five people and any number of AI agents.

**This is a solo run**, decided Thu Nov 12: one developer, handle `lead`, owns every stream and directs the AI agents. The rest of this note applies if someone joins, and its folder rules still apply between agents working at the same time.

| Handle | Streams | Engine 5.8.3 installed |
|---|---|---|
| lead | all | yes |

## How to split the work

- A person takes whole streams, not scattered tasks. Streams own separate folders, so two people in different streams cannot overwrite each other.
- Suggested split by team size:

| People | Split |
|---|---|
| 2 | core + fight + cam · stage + feel + ship |
| 3 | core + ship · fight + cam · stage + feel |
| 4 | core + ship · fight · cam · stage + feel |
| 5 | core + ship · fight · cam · stage · feel |

- Each person can run AI agents on their own streams. Two agents never work in the same stream folder at the same time.
- The lead owns the shared files listed in [[CLAUDE]], assigns tasks at the start of each sprint, and runs every integration checkpoint.

## Setting up a machine

1. Install Unreal Engine 5.8.3 from the Epic launcher, Visual Studio with the C++ game development workload, Git and Git LFS.
2. Clone the repository and run `git lfs pull`.
3. Run `Tools/check.ps1`. It must print `PASS`. It also switches on the push guard for `main` on this machine.
4. Open `StickToTheScript.uproject`. You should see nothing broken in the Output Log.
5. Open this folder as an Obsidian vault to read the plan.
