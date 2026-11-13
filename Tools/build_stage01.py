# Builds Content/Scenes/Stage01: a long warehouse the fight runs through, with cover for opponents to step out from.
# The level is generated, so this script is its source. Hand edits made in the editor are lost when it runs again.
# Where the opponents wait and fight is in Content/Scripts/Scene01.json; the cover here is placed to hide them.
#
# Run with the editor closed, after Tools/build_materials.py:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import sys

import unreal

sys.path.append(unreal.Paths.project_dir() + "Tools")
from stage_kit import (CUBE, CYLINDER, begin, block, cosmetic, crate, fill_light, film_cameras, finish, glow, grade, haze, marks,
                       material, room, sign, spot_light, surface)

LEVEL = "/Game/Scenes/Stage01"
begin(LEVEL)

floor_look = surface("MI_SetFloor", "Concrete", (0.6, 0.6, 0.64), 400.0)
wall_look = surface("MI_SetWall", "Brick", (0.9, 0.85, 0.8), 200.0)
roof_look = surface("MI_SetRoof", "Metal", (0.2, 0.22, 0.25), 200.0)
crate_look = surface("MI_SetCrate", "Planks", (1.5, 1.3, 1.0), 220.0)
frame_look = surface("MI_SetCrateFrame", "Planks", (0.75, 0.6, 0.45), 90.0)
blue_steel = surface("MI_SetContainerBlue", "Metal", (0.2, 0.42, 0.75), 130.0)
red_steel = surface("MI_SetContainerRed", "Metal", (0.85, 0.24, 0.16), 130.0)
green_steel = surface("MI_SetContainerGreen", "Metal", (0.2, 0.55, 0.32), 130.0)
drum_look = surface("MI_SetDrum", "Metal", (0.25, 0.4, 0.6), 60.0, 0.5)
beam_look = material("MI_SetBeam", 0.1, 0.12, 0.15)
tape_look = material("MI_SetTape", 0.85, 0.7, 0.05)
metal_look = material("MI_SetMetal", 0.05, 0.05, 0.06)
daylight = glow("MI_SetWindow", 0.5, 0.68, 1.0, 34.0)
bulb = glow("MI_SetBulb", 1.0, 0.8, 0.5, 220.0)

# 30 m long, 13 m deep, 5.5 m high. The fight starts at the origin and runs along +X, filmed from the +Y side.
room((850, 200), (3000, 1300, 550), floor_look, wall_look, roof_look)

marks({"PlayerMark": (0, 0, 0)}, tape_look)

# The wide camera travels with the player. The final cut carries the other two to each strike.
film_cameras({
    "Cam_Wide": ((0, 380, 100), (0, 0, 100), 80.0),
    "Cam_Close": ((0, 260, 145), (0, 0, 145), 55.0),
    "Cam_Low": ((-70, 170, 25), (0, 0, 130), 70.0),
})

# Crate stacks between the path and the back wall. Opponents wait behind them and step out for their part.
for index, x in enumerate((390, 710)):
    crate(f"Cover_{index}_Low", crate_look, frame_look, (x, -235, 60), (2.6, 0.9, 1.2), 3.0)
    crate(f"Cover_{index}_High", crate_look, frame_look, (x - 20, -240, 170), (2.2, 0.85, 1.0), -4.0)

# The back wall is what the camera looks at for the whole fight, so it carries the building: steel columns under a beam,
# and between them high windows that let the day in as shafts through the haze.
for index, x in enumerate(range(-600, 2401, 600)):
    block(f"Column_{index}", CUBE, beam_look, (x, -428, 275), (0.32, 0.32, 5.5))
block("Beam_Back", CUBE, beam_look, (850, -428, 505), (30.0, 0.3, 0.4))
for index, x in enumerate(range(-300, 2101, 600)):
    block(f"Window_{index}", CUBE, daylight, (x, -447, 390), (2.8, 0.04, 1.5))
    block(f"Window_{index}_Bar_Across", CUBE, beam_look, (x, -444, 390), (2.8, 0.03, 0.06))
    for bar, offset in enumerate((-47, 47)):
        block(f"Window_{index}_Bar_{bar}", CUBE, beam_look, (x + offset, -444, 390), (0.06, 0.03, 1.5))
    shaft = spot_light(f"Light_Window_{index}", (x, -436, 395), (x + 70, -40, 0), 5000.0, (0.72, 0.84, 1.0), 26.0)
    cosmetic(shaft.light_component, "volumetric_scattering_intensity", 4.0)

# Pools of light down the length of the room: a warm key from the camera side, a cool rim from behind.
for index, x in enumerate((100, 650, 1200, 1750)):
    spot_light(f"Light_Key_{index}", (x - 260, 330, 430), (x, 0, 110), 9000.0, (1.0, 0.86, 0.68), 36.0)
    spot_light(f"Light_Rim_{index}", (x + 330, -340, 450), (x, 0, 120), 6000.0, (0.6, 0.75, 1.0), 34.0)
    # The stands are behind the travelling camera's track. In front of it they cross the picture as black bars.
    block(f"Stand_Key_{index}", CYLINDER, metal_look, (x - 260, 460, 215), (0.06, 0.06, 4.3))
# One more pool over the left end of the arena, with its lamps turned round because the wall is too close on that side.
# Without it a fight that backs away from the first opponents ends up in the dark.
spot_light("Light_Key_End", (-190, 330, 430), (-450, 0, 110), 9000.0, (1.0, 0.86, 0.68), 36.0)
spot_light("Light_Rim_End", (-120, -340, 450), (-450, 0, 120), 6000.0, (0.6, 0.75, 1.0), 34.0)
block("Stand_Key_End", CYLINDER, metal_look, (-190, 460, 215), (0.06, 0.06, 4.3))
fill_light((850, 150, 535), 2600.0, 1400.0)

# Lamps hanging over the goods at the back: a shade, a bulb that shows, and a warm pool under each. They cast no shadows,
# which keeps them cheap; the key and rim lights do that work.
for index, x in enumerate((0, 600, 1200, 1800)):
    block(f"Lamp_{index}_Cord", CYLINDER, metal_look, (x, -300, 495), (0.015, 0.015, 1.1))
    block(f"Lamp_{index}_Shade", CYLINDER, metal_look, (x, -300, 432), (0.6, 0.6, 0.16))
    block(f"Lamp_{index}_Bulb", CYLINDER, bulb, (x, -300, 422), (0.3, 0.3, 0.04))
    spot_light(f"Light_Lamp_{index}", (x, -300, 418), (x, -300, 0), 3500.0, (1.0, 0.78, 0.5), 55.0, shadows=False)

haze(0.012)
grade(6.0, 0.6)

# Goods along the back wall, kept clear of the path and of the two places where opponents wait (x 480 to 960, behind the cover).
block("Container_Blue", CUBE, blue_steel, (40, -325, 125), (5.6, 2.3, 2.5))
block("Container_Red", CUBE, red_steel, (1960, -325, 125), (5.6, 2.3, 2.5))
block("Container_Green", CUBE, green_steel, (2060, -330, 375), (5.6, 2.3, 2.5), 2.0)
crate("Crate_1", crate_look, frame_look, (-470, -360, 55), (1.1, 1.1, 1.1), 8.0)
crate("Crate_2", crate_look, frame_look, (-455, -365, 150), (0.8, 0.8, 0.8), -14.0)
crate("Crate_3", crate_look, frame_look, (1080, -380, 50), (1.0, 1.0, 1.0), -6.0)
crate("Crate_4", crate_look, frame_look, (1090, -375, 145), (0.9, 0.9, 0.9), 11.0)
crate("Crate_5", crate_look, frame_look, (1215, -385, 40), (0.8, 0.8, 0.8), 4.0)
for index, (x, y) in enumerate(((1380, -400), (1450, -395), (1520, -402), (1415, -330))):
    block(f"Drum_{index}", CYLINDER, drum_look, (x, y, 55), (0.6, 0.6, 1.1))
    block(f"Drum_{index}_Lid", CYLINDER, metal_look, (x, y, 110.5), (0.56, 0.56, 0.01))

# A walkway painted on the floor behind the path, and the bay's number on the wall.
block("Line_Back", CUBE, tape_look, (850, -190, 0.3), (30.0, 0.08, 0.005))
sign("Sign_Bay", "BAY 7", (900, -446, 250), 90.0, (0.9, 0.9, 0.85), 14.0)

finish(LEVEL)
