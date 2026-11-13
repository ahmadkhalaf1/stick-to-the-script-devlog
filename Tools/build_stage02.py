# Builds Content/Scenes/Stage02: a bar, for a fight that travels along the counter against four.
# The level is generated, so this script is its source. Hand edits made in the editor are lost when it runs again.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import sys

import unreal

sys.path.append(unreal.Paths.project_dir() + "Tools")
from stage_kit import CUBE, CYLINDER, begin, block, fill_light, film_cameras, finish, glow, grade, haze, marks, material, room, sign, spot_light, surface

LEVEL = "/Game/Scenes/Stage02"
begin(LEVEL)

floor_look = surface("MI_BarFloor", "Planks", (1.3, 1.0, 0.8), 200.0)
wall_look = surface("MI_BarWall", "Wallpaper", (0.75, 0.2, 0.18), 160.0)
wood_look = surface("MI_BarWood", "Planks", (0.7, 0.45, 0.32), 110.0)
top_look = surface("MI_BarTop", "Planks", (1.6, 1.1, 0.75), 90.0, 0.4)
tape_look = material("MI_SetTape", 0.85, 0.7, 0.05)
metal_look = material("MI_SetMetal", 0.05, 0.05, 0.06)
bottle_looks = [
    material("MI_BottleGreen", 0.02, 0.22, 0.06),
    material("MI_BottleAmber", 0.5, 0.25, 0.03),
    material("MI_BottleClear", 0.6, 0.65, 0.7),
]

# 16 m wide, 13 m deep, 5 m high. The fight happens at the origin, filmed from the +Y side.
room((0, 200), (1600, 1300, 500), floor_look, wall_look, wood_look)

# Where the opponents wait and fight is in Content/Scripts/Scene02.json: two start at the bar, two come in from the ends of the room.
marks({"PlayerMark": (0, 0, 0)}, tape_look)

# The wide camera travels with the player, further back than on stage 1. The final cut carries the other two to each strike.
film_cameras({
    "Cam_Wide": ((0, 430, 105), (0, 0, 105), 80.0),
    "Cam_Close": ((0, 280, 145), (0, 0, 145), 55.0),
    "Cam_Low": ((-30, 190, 25), (0, 0, 130), 70.0),
})

# The bar along the back wall, with a shelf of bottles behind it and stools in front.
block("Bar_Counter", CUBE, wood_look, (0, -340, 55), (10.0, 0.8, 1.1))
block("Bar_Top", CUBE, top_look, (0, -335, 113), (10.2, 1.0, 0.06))
for shelf, height in enumerate((180, 245)):
    block(f"Shelf_{shelf}", CUBE, wood_look, (0, -435, height), (10.0, 0.25, 0.06))
    for index, x in enumerate(range(-450, 451, 60)):
        block(f"Bottle_{shelf}_{index}", CYLINDER, bottle_looks[(index + shelf) % 3], (x, -435, height + 18), (0.09, 0.09, 0.3))
for index, x in enumerate(range(-400, 401, 200)):
    block(f"Stool_{index}_Seat", CYLINDER, top_look, (x, -255, 78), (0.4, 0.4, 0.06))
    block(f"Stool_{index}_Leg", CYLINDER, metal_look, (x, -255, 38), (0.07, 0.07, 0.75))

# Tables and chairs at the sides, out of the way of the fight.
for index, (x, y) in enumerate(((-520, 130), (540, 110))):
    block(f"Table_{index}_Top", CYLINDER, top_look, (x, y, 76), (1.1, 1.1, 0.05))
    block(f"Table_{index}_Leg", CYLINDER, metal_look, (x, y, 37), (0.1, 0.1, 0.74))
    block(f"Chair_{index}_A", CUBE, wood_look, (x - 95, y + 20, 23), (0.45, 0.45, 0.46), 15.0)
    block(f"Chair_{index}_B", CUBE, wood_look, (x + 60, y - 85, 23), (0.45, 0.45, 0.46), -30.0)

# Three warm lamps in a row over the floor, the brightest in the middle. The two over the ends of the arena are there
# because a fighter at an end was lit only from behind by the coloured glow, and showed as a dark shape in the film.
for name, x, candelas in (("Middle", 0, 8000.0), ("Left", -380, 5500.0), ("Right", 380, 5500.0)):
    spot_light(f"Light_Lamp_{name}", (x, 90, 430), (x, 0, 100), candelas, (1.0, 0.8, 0.55), 46.0)
    block(f"Lamp_Shade_{name}", CYLINDER, metal_look, (x, 90, 445), (0.7, 0.7, 0.2))
    block(f"Lamp_Bulb_{name}", CYLINDER, glow("MI_BarBulb", 1.0, 0.75, 0.45, 220.0), (x, 90, 434), (0.34, 0.34, 0.03))
# A red glow from one end of the bar and a teal one from the other, and light on the bottles.
spot_light("Light_Neon_Red", (560, -300, 300), (0, 0, 120), 6000.0, (1.0, 0.15, 0.3), 40.0)
spot_light("Light_Neon_Teal", (-560, -300, 300), (0, 0, 120), 4500.0, (0.2, 0.7, 1.0), 40.0)
for index, x in enumerate((-300, 0, 300)):
    spot_light(f"Light_Shelf_{index}", (x, -330, 380), (x, -435, 200), 2500.0, (1.0, 0.7, 0.35), 50.0)
# The glow has its lamps on show: a red tube over one half of the shelves, a teal one over the other, and the name between them.
block("Neon_Red", CYLINDER, glow("MI_BarNeonRed", 1.0, 0.12, 0.25, 90.0), (290, -444, 300), (0.06, 0.06, 3.6), 0.0, 90.0)
block("Neon_Teal", CYLINDER, glow("MI_BarNeonTeal", 0.15, 0.75, 1.0, 70.0), (-290, -444, 300), (0.06, 0.06, 3.6), 0.0, 90.0)
sign("Sign_Bar", "BAR", (0, -446, 330), 110.0, (1.0, 0.25, 0.5), 120.0)
fill_light((0, 150, 485), 900.0, 700.0, (1.0, 0.85, 0.7))
haze(0.015)
grade(6.0, 0.7)

finish(LEVEL)
