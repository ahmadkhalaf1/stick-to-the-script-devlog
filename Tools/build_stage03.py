# Builds Content/Scenes/Stage03: a rooftop at night, with the city behind it and a helipad at the far end.
# The level is generated, so this script is its source. Hand edits made in the editor are lost when it runs again.
# Where the opponents wait and fight, and where the captive is, is in Content/Scripts/Scene03.json.
#
# Run with the editor closed, after Tools/build_materials.py and Tools/build_scans.py:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import sys

import unreal

sys.path.append(unreal.Paths.project_dir() + "Tools")
from stage_kit import (CUBE, CYLINDER, SPHERE, begin, block, cosmetic, dress, fill_light, film_cameras, finish, glow, grade, haze, marks,
                       material, sky, spot_light, surface)

LEVEL = "/Game/Scenes/Stage03"
begin(LEVEL)

# The surfaces are photographs, and a tint multiplies one.
roof_look = surface("MI_RoofFloor", "Roofing", (1.7, 1.7, 1.9), 220.0)
parapet_look = surface("MI_RoofWall", "Concrete", (1.8, 1.8, 2.05), 300.0)
unit_look = surface("MI_RoofUnit", "Steel", (1.2, 1.2, 1.2), 200.0)
tower_look = surface("MI_RoofTower", "Concrete", (0.5, 0.55, 0.7), 700.0)
window_look = glow("MI_RoofWindow", 1.0, 0.8, 0.45, 26.0)
tape_look = material("MI_SetTape", 0.85, 0.7, 0.05)
metal_look = material("MI_SetMetal", 0.05, 0.05, 0.06)

# The roof is 30 m long and 12 m deep, with no walls and no ceiling: night all around, under a photographed sky of
# moonlit cloud. There is nothing under its horizon, so that is let down behind the parapet.
sky("MI_RoofSky", (900, 100, 0), 5.0, 301.0, 8.0)

# The fight starts at the origin and runs along +X towards the helipad, filmed from the +Y side.
block("Roof", CUBE, roof_look, (900, 100, -10), (30.0, 12.0, 0.2))
block("Parapet_Back", CUBE, parapet_look, (900, -485, 45), (30.0, 0.3, 0.9))
block("Parapet_Left", CUBE, parapet_look, (-585, 100, 45), (0.3, 12.0, 0.9))
block("Parapet_Right", CUBE, parapet_look, (2385, 100, 45), (0.3, 12.0, 0.9))

marks({"PlayerMark": (0, 0, 0)}, tape_look)

# The wide camera travels with the player. The final cut carries the other two to each strike.
film_cameras({
    "Cam_Wide": ((0, 430, 105), (0, 0, 105), 80.0),
    "Cam_Close": ((0, 280, 145), (0, 0, 145), 55.0),
    "Cam_Low": ((-30, 190, 25), (0, 0, 130), 70.0),
})

# The stair house the hero came up through, behind the start.
block("StairHouse", CUBE, parapet_look, (-380, -300, 130), (2.6, 2.6, 2.6))
block("StairHouse_Door", CUBE, metal_look, (-248, -300, 100), (0.05, 1.0, 2.0))

# Roof machinery along the back, clear of the path: air units, vents, a water tank, an aerial.
for index, x in enumerate((250, 980, 1620)):
    dress(f"AirUnits_{index}", "AirUnits", (x, -340, 0), 0.0, 1.3)
for index, x in enumerate((520, 1300, 1900)):
    block(f"Vent_{index}", CYLINDER, unit_look, (x, -400, 45), (0.45, 0.45, 0.9))
block("Tank", CYLINDER, unit_look, (680, -390, 260), (2.0, 2.0, 2.2))
for index, (x, y) in enumerate(((610, -330), (750, -330), (610, -450), (750, -450))):
    block(f"Tank_Leg_{index}", CYLINDER, metal_look, (x, y, 75), (0.1, 0.1, 1.5))
block("Aerial", CYLINDER, metal_look, (1250, -440, 330), (0.05, 0.05, 6.6))
block("Aerial_Beacon", SPHERE, glow("MI_RoofBeacon", 1.0, 0.05, 0.05, 260.0), (1250, -440, 668), (0.22, 0.22, 0.22))
dress("StairHouse_Lamp_Body", "WallLamp", (-243, -300, 195), -90.0)
block("StairHouse_Lamp", SPHERE, glow("MI_RoofLamp", 1.0, 0.8, 0.5, 160.0), (-240, -300, 216), (0.1, 0.1, 0.1))

# The lamp over the door lights the stair house and what hangs on it, which the work lights do not reach.
spot_light("Light_StairHouse", (-190, -110, 270), (-330, -240, 70), 2600.0, (1.0, 0.8, 0.55), 62.0, shadows=False)

# What a roof collects: switchgear on the stair house, gas bottles under the tank, a bin, barriers, tyres, crates.
dress("FuseBox", "FuseBox", (-380, -165, 110))
dress("Cabinet_Blue", "CabinetBlue", (-470, -148, 0))
dress("Camera", "Camera", (-262, -175, 235), 20.0)
dress("Cabinet_Green", "CabinetGreen", (1130, -448, 0))
dress("GasBottle_0", "GasBottle", (560, -360, 0))
dress("GasBottle_1", "GasBottle", (598, -385, 0), 70.0)
dress("Bin", "Bin", (120, -435, 0), 15.0)
dress("Barrier_0", "Barrier", (1790, -430, 0), 4.0)
dress("Barrier_1", "Barrier", (1960, -435, 0), -3.0)
for index in range(2):
    dress(f"Tyre_{index}", "Tyre", (1480 + 5 * index, -430, 17 * index), 40.0 * index)
    dress(f"PlasticCrate_{index}", "PlasticCrate", (400 + 3 * index, -420, 25 * index), 12.0 * index)

# The helipad at the far end, where the captive waits: a painted circle with an H.
block("Helipad", CYLINDER, tape_look, (2120, 0, 0.3), (3.6, 3.6, 0.005))
block("Helipad_Inner", CYLINDER, roof_look, (2120, 0, 0.5), (3.3, 3.3, 0.005))
block("Helipad_H_Left", CUBE, tape_look, (2120, -50, 0.7), (1.4, 0.2, 0.005))
block("Helipad_H_Right", CUBE, tape_look, (2120, 50, 0.7), (1.4, 0.2, 0.005))
block("Helipad_H_Bar", CUBE, tape_look, (2120, 0, 0.7), (0.2, 1.0, 0.005))
pad_light = glow("MI_RoofPadLight", 0.3, 1.0, 0.5, 120.0)
for index, (dx, dy) in enumerate(((1, 0), (0.7, 0.7), (0, 1), (-0.7, 0.7), (-1, 0), (-0.7, -0.7), (0, -1), (0.7, -0.7))):
    block(f"Helipad_Light_{index}", CYLINDER, pad_light, (2120 + 172 * dx, 172 * dy, 1.5), (0.1, 0.1, 0.03))

# The city behind: towers of different heights, each with a few lit windows. Their tops are above the roof and
# their feet far below it, so the roof reads as high up.
towers = ((-700, -1500, 420, 500), (-150, -1900, 520, 1300), (450, -1450, 380, 250), (1000, -2100, 600, 1700),
          (1550, -1500, 440, 650), (2150, -1800, 500, 1100), (2800, -1500, 400, 350))
for index, (x, y, width, top) in enumerate(towers):
    block(f"Tower_{index}", CUBE, tower_look, (x, y, top / 2 - 1250), (width / 100, width / 100, (top + 2500) / 100))
    for row in range(4):
        for column in range(3):
            if (index + row * 2 + column) % 3 == 0:
                continue
            block(f"Window_{index}_{row}_{column}", CUBE, window_look,
                  (x - width / 4 + column * width / 4, y + width / 2 + 3, top - 120 - row * 190), (0.5, 0.03, 0.8))

# Two cool lights for the city, so the towers and their windows show against the dark. They sit below the roof and
# behind its back edge: from anywhere else they light the roof as well, and wash the whole set out.
for index, x in enumerate((200, 1700)):
    city = spot_light(f"Light_City_{index}", (x, -650, -250), (x, -1700, 450), 12000.0, (0.6, 0.68, 1.0), 72.0)
    city.light_component.set_attenuation_radius(4000.0)
    cosmetic(city.light_component, "volumetric_scattering_intensity", 0.0)

# On the roof: warm work lights from the camera side, a cool rim from behind, and a red warning light on the aerial.
for index, x in enumerate((100, 650, 1200, 1750)):
    spot_light(f"Light_Key_{index}", (x - 260, 330, 430), (x, 0, 110), 8000.0, (1.0, 0.86, 0.68), 36.0)
    spot_light(f"Light_Rim_{index}", (x + 330, -340, 450), (x, 0, 120), 6500.0, (0.5, 0.65, 1.0), 34.0)
    # The stands are behind the travelling camera's track. In front of it they cross the picture as black bars.
    block(f"Stand_Key_{index}", CYLINDER, metal_look, (x - 260, 460, 215), (0.06, 0.06, 4.3))
spot_light("Light_Beacon", (1250, -430, 640), (1250, -150, 0), 7000.0, (1.0, 0.08, 0.08), 55.0)
spot_light("Light_Helipad", (2120, 330, 480), (2120, 0, 60), 9000.0, (1.0, 0.9, 0.7), 40.0)
fill_light((900, 150, 700), 2200.0, 1400.0, (0.5, 0.6, 1.0))
haze(0.012, 5000.0)
grade(6.0, 0.6)

finish(LEVEL)
