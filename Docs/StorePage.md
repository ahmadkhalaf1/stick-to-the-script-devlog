# Store page draft (itch.io)

A draft. Nothing here is published yet. The text below the first rule is meant to be pasted into the page; the lists at the end are for whoever fills in the page and the jam's form.

---

## Title

Stick to the Script

## Short description

You are the stunt performer. The script says you lose. Ignore it, win the fight, then cut the film yourself.

## Description

**A playable movie for the 2026 Epic AI MegaJam. Theme: Director's Cut.**

You were hired to lose. The script says the gang beats you in the warehouse, in the bar and on the rooftop. You have other ideas.

- **A free fight, not a string of prompts.** Walk, punch, kick, duck, guard and jump. Duck and punch for an uppercut, duck and kick for a sweep, guard and punch for a hook, guard and kick for a spinning roundhouse. Jump under a red bar to hang from it, then kick off it. Throw whatever is lying on the set: crates, bottles, a watermelon, a wet-floor sign, a frying pan. The keys stay along the bottom of the screen and say what each one does at that moment.
- **The big finish.** Every blow you land fills a bar. When it is full the screen shows you a key: press it, and a hook, an uppercut and a spinning kick, in slow motion and each from its own camera, take everyone near you apart.
- **Guns, if you get to them first.** A pistol in the warehouse, a rifle on the roof, and gunmen whose aim you can see and duck. Put a gunman down and his pistol is yours.
- **Go down and it is "Cut!"** Back to one, and every retake comes out of your fee.
- **Win and it is a wrap.** The take is played back as a film, and the cut is yours: the action cut, the slow-motion cut, or one long take. After it come the outtakes: the takes you lost, as they were shot.
- **The box office pays for a good show.** The same punch over and over earns nothing. Mix your moves.
- **A director with a voice**, and different things to shout in every scene.
- **Something golden** drops onto the set once in every take. Pick it up.
- **A prop shop** for what you bank: a bat, an iron bar, a sledgehammer, a pistol or a rifle for your next scene, and a halo, a crown or a new colour for good.
- Three scenes that get harder, five kinds of opponent and a boss on the roof, a wardrobe, and a director who cannot believe what you are doing to his stuntmen.

## Controls

| | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | W A S D or the arrow keys | Left stick |
| Punch | Left mouse button, or J | X |
| Kick | F, or K | Y |
| Guard (hold) | Right mouse button, or Shift or L | Left trigger |
| Duck (hold) | Ctrl, or C | Right trigger |
| Jump | Space | A |
| Grab and throw | E | B |
| The big finish, when its bar is full | Q | Right bumper |
| Fire a gun in your hands (hold) | Left mouse button, or J | X |
| Change the view | V | |
| Pause | Esc, or P | Start |
| Uppercut, sweep | hold duck, then punch or kick | |
| Hook, roundhouse | hold guard, then punch or kick | |
| Off the bar | jump under a red bar, then kick | |

In menus: point and click, or choose with W, A, S and D and press Enter.

## How to run

Windows 10 or 11. Unzip, open the `StickToTheScript` folder and run `StickToTheScript.exe`.

## How it was made

Made during the jam, Nov 12 to 15, 2026, by one developer directing an AI coding agent (Claude Code). The agent wrote the code, drove the Unreal editor and kept a log of everything as it happened, including what went wrong. The dev log, with the scripts and the source code: https://github.com/ahmadkhalaf1/stick-to-the-script-devlog

No art was bought. The fighters' body is Mixamo's free "X Bot" model (Adobe), which the game poses itself, bone by bone, on sixteen joints. The punch, the kicks, the uppercut, the fighting stance and the way a blow is taken are free motion-capture clips from Mixamo (Adobe), fitted to that skeleton by a script. The sets are built by scripts. Their brick, boards and concrete, the night sky over the roof, the guns, most of the props and the furniture are free scans and photographs from Poly Haven (CC0). The director's voice, the blows and the music were generated with ElevenLabs models through fal.ai, a partner of the jam, from words that are in the repository.

## Third-party assets

The Unreal Engine mannequin, whose skeleton the fighters are worked out on, and the engine's Roboto font. From Mixamo (Adobe): the "X Bot" character model and nine fight animation clips. From Poly Haven, all CC0: 41 scanned models (the guns, the props and what dresses the sets), nine photographed surfaces and one night sky. Nothing from Fab. Every one is named, with its licence, in the dev log's register.

## AI disclosure (the page's own field)

Made by one developer directing an AI coding agent (Claude Code). The agent wrote the code, the scripts that build the sets, and the game's text. The director's voice, the sound effects and the music were generated with ElevenLabs models through fal.ai. The character model and its animations (Mixamo) and the scanned props, surfaces and sky (Poly Haven) are not AI-made. The dev log, the scripts and the prompts are public: https://github.com/ahmadkhalaf1/stick-to-the-script-devlog

---

## Before the page goes public

The page exists as a draft since Fri Nov 13: https://the-fighter-ps.itch.io/stick-to-the-script (only the developer can see it). Its text, genre, tags and AI disclosure are filled in from this file, last on Sat Nov 14.

- [x] Cover, screenshots and the zip uploaded to the draft, on Fri Nov 13. The zip there is replaced whenever the game changes, last on Sat Nov 14 at 18:35, when the six pictures of Nov 13 also gave way to six of that evening's. The page's upload buttons open a file chooser; `Toolschoose_file.ps1` fills it in

- [x] Team name: Page Twelve. The zip is `PageTwelve_StickToTheScript.zip`, made by `Tools\release.ps1 -Team PageTwelve`
- [ ] Screenshots taken from the packaged game. Eight are in `Docs/store`, taken at 1920 by 1080 from the gameplay video of the self-played run of Sat Nov 14, 18:15, the eighth being the slate that ends a film. The strip along the top, where a tool on the developer's PC draws its frame counter, is cut off. Six of them are on the draft page since Sat Nov 14, 18:41, in this order: warehouse, bar, rooftop, the choice of cut, a film shot, the slate; `6-film-bar` and `7-film-rooftop` were left off. The developer has not chosen among them
- [x] A cover image, 630 by 500: `Docs/store/cover.png`
- [x] The link to the 30 to 60 second gameplay video: https://www.youtube.com/watch?v=HJW6ExPfZdc (58 seconds, a self-played run of the build of Sat Nov 14, cut at 18:28 and uploaded by the developer that evening). It is on the draft page too, and YouTube shows it without a sign-in. The video before it, of Fri Nov 13, was https://www.youtube.com/watch?v=zvQRRTC56iA. It is made again if the game changes much
- [x] A public link for the dev log: https://github.com/ahmadkhalaf1/stick-to-the-script-devlog. It is a copy of this repository without the binary assets and without the history, refreshed by `Tools\publish_devlog.ps1`
- [x] The gamepad column: no gamepad has been tried, so the page on itch.io lists keyboard and mouse only
- [ ] The developer has read this text and agrees with it

## For the jam's form

| Field | What goes in |
|---|---|
| Team name | Page Twelve. Members and Epic-account emails are collected outside the repository |
| Game name | Stick to the Script |
| Gameplay video | https://www.youtube.com/watch?v=HJW6ExPfZdc |
| Third-party assets | The Unreal Engine mannequin: meshes, rigs and animations from the engine's template content. The engine's Roboto font, for the lettering on the sets. From Mixamo (Adobe), free with an Adobe account and royalty-free inside a game: the "X Bot" character model, and eleven fight animation clips of which the game uses nine. Thirteen scanned models from Poly Haven (polyhaven.com), all CC0: Service Pistol (`service_pistol`), Bolt Action Rifle 7.62 (`bolt_action_rifle_7_62`), Baseball Bat (`baseball_bat`), Crowbar 01 (`crowbar_01`), Sledgehammer 01 (`sledgehammer_01`), Ornate War Hammer (`ornate_war_hammer`), Antique Katana 01 (`antique_katana_01`), Wooden Crate 02 (`wooden_crate_02`), Brass Pan 01 (`brass_pan_01`), Wet Floor Sign 01 (`WetFloorSign_01`), Bar Chair Round 01 (`bar_chair_round_01`), Wine Bottles 01 (`wine_bottles_01`), Stick Grenade (`stick_grenade`). Twenty-eight more scanned models from Poly Haven, all CC0, that dress the sets: Barrel 01 (`Barrel_01`), Barrel 03 (`barrel_03`), Old Military Crate (`old_military_crate`), Wooden Military Crate (`wooden_military_crate`), Cardboard Box 01 (`cardboard_box_01`), Steel Frame Shelves 01 (`steel_frame_shelves_01`), Old Tyre (`old_tyre`), Metal Tool Chest (`metal_tool_chest`), Korean Fire Extinguisher 01 (`korean_fire_extinguisher_01`), Utility Box 01 (`utility_box_01`), Utility Box 02 (`utility_box_02`), Power Box 01 (`power_box_01`), Hanging Industrial Lamp (`hanging_industrial_lamp`), Portable Generator (`portable_generator`), Plastic Crate 02 (`plastic_crate_02`), Round Wooden Table 01 (`round_wooden_table_01`), Green Chair 01 (`GreenChair_01`), Vintage Cabinet 01 (`vintage_cabinet_01`), Ceiling Fan (`ceiling_fan`), Television 02 (`television_02`), Standing Chalkboard 01 (`standing_chalkboard_01`), Wine Barrel 01 (`wine_barrel_01`), Exterior Aircon Unit (`exterior_aircon_unit`), Propane Tank (`propane_tank`), Security Camera 01 (`security_camera_01`), Industrial Wall Lamp (`industrial_wall_lamp`), Metal Trash Can (`metal_trash_can`), Concrete Road Barrier (`concrete_road_barrier`). Nine photographed surfaces from Poly Haven, all CC0: Concrete Floor Worn 001 (`concrete_floor_worn_001`), Factory Brick (`factory_brick`), Wood Floor Worn (`wood_floor_worn`), Decrepit Wallpaper (`decrepit_wallpaper`), Tarred Gravel (`tarred_gravel`), Wooden Planks (`wooden_planks`), Dark Wooden Planks (`dark_wooden_planks`), Corrugated Iron 02 (`corrugated_iron_02`), Rusty Metal 04 (`rusty_metal_04`). One sky from Poly Haven, CC0: Kloppenheim 07 (Pure Sky) (`kloppenheim_07_puresky`). Nothing from Fab |
| AI and other tools | Claude Code with the model Claude Fable 5.2 (research, code, editor automation, dev log). fal.ai with ElevenLabs Sound Effects v3, Eleven v3 speech and ElevenLabs Music (the recorded blows, the director's voice, the music), and Whisper (to check the spoken lines). Unreal's MCP and toolset plugins (editor only, not in the build). The Claude Code plugins ponytail, Karpathy guidelines and claude-code-setup (agent behaviour only) |
| Made before Nov 12 | Nothing |
| Dev log | https://github.com/ahmadkhalaf1/stick-to-the-script-devlog |
