# Builds every prop in Content/Props as a real mesh, and the one material they share.
# The props are generated, so this script is their source. Hand edits made in the editor are lost when it runs again.
#
# Run with the editor closed. Geometry Script is an editor plugin that the project does not enable, so it is asked for here:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -EnablePlugins=GeometryScripting -unattended -nosplash -nullrhi -stdout

import math

import unreal

FOLDER = "/Game/Props"
assets = unreal.EditorAssetLibrary
shapes = unreal.GeometryScript_Primitives
edits = unreal.GeometryScript_MeshEdits
colours = unreal.GeometryScript_VertexColors
CENTER = unreal.GeometryScriptPrimitiveOriginMode.CENTER
BASE = unreal.GeometryScriptPrimitiveOriginMode.BASE


def at(x=0.0, y=0.0, z=0.0, pitch=0.0, yaw=0.0, roll=0.0, scale=(1.0, 1.0, 1.0)):
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), unreal.Vector(*scale))


# A colour is red, green, blue and how much of a metal the surface is. The material reads the fourth as shine.
WOOD = (0.42, 0.26, 0.12, 0.0)
DARK_WOOD = (0.2, 0.11, 0.05, 0.0)
PALE_WOOD = (0.62, 0.45, 0.24, 0.0)
STEEL = (0.32, 0.33, 0.36, 1.0)
BLACK_IRON = (0.06, 0.06, 0.07, 1.0)
GLASS = (0.03, 0.3, 0.08, 0.7)
LABEL = (0.85, 0.8, 0.62, 0.0)
ORANGE = (0.95, 0.3, 0.03, 0.0)
WHITE = (0.9, 0.9, 0.88, 0.0)
MELON = (0.07, 0.36, 0.08, 0.1)
MELON_STRIPE = (0.02, 0.17, 0.03, 0.1)
GOLD = (1.0, 0.72, 0.16, 1.0)
DEEP_GOLD = (0.7, 0.42, 0.06, 1.0)
TAPE = (0.05, 0.05, 0.05, 0.0)


def painted(mesh, colour):
    flags = unreal.GeometryScriptColorFlags()
    colours.set_mesh_constant_vertex_color(mesh, unreal.LinearColor(*colour), flags, True)
    return mesh


def part(build, colour):
    """One piece of a prop: built on its own, painted, and handed back to be joined on."""
    mesh = unreal.DynamicMesh()
    build(mesh, unreal.GeometryScriptPrimitiveOptions())
    return painted(mesh, colour)


def box(colour, size, where):
    return part(lambda mesh, options: shapes.append_box(mesh, options, where, size[0], size[1], size[2], 0, 0, 0, CENTER), colour)


def rod(colour, radius, length, where, sides=14):
    return part(lambda mesh, options: shapes.append_cylinder(mesh, options, where, radius, length, sides, 0, True, BASE), colour)


def ball(colour, radius, where):
    return part(lambda mesh, options: shapes.append_sphere_lat_long(mesh, options, where, radius, 12, 18, CENTER), colour)


def taper(colour, bottom, top, length, where, sides=14):
    return part(lambda mesh, options: shapes.append_cone(mesh, options, where, bottom, top, length, sides, 0, True, BASE), colour)


def turned(colour, profile, where=None, steps=20):
    """A shape made on a lathe: the profile is (radius, height) pairs, bottom to top."""
    path = [unreal.Vector2D(radius, height) for radius, height in profile]
    return part(lambda mesh, options: shapes.append_revolve_path(mesh, options, where or at(), path, unreal.GeometryScriptRevolveOptions(), steps, True), colour)


def save(name, pieces, lying=False):
    whole = unreal.DynamicMesh()
    for piece in pieces:
        edits.append_mesh(whole, piece, at())
    if lying:
        # Built standing up, kept lying along X: that is how a bat rests on a floor, and the game holds it by that axis.
        unreal.GeometryScript_MeshTransforms.rotate_mesh(whole, unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0), unreal.Vector(0, 0, 0))

    # The pivot goes to the middle, which is where the game holds and spins a prop.
    box_of = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(whole)
    middle = (box_of.min + box_of.max) * 0.5
    unreal.GeometryScript_MeshTransforms.translate_mesh(whole, unreal.Vector(-middle.x, -middle.y, -middle.z))

    path = f"{FOLDER}/{name}"
    if assets.does_asset_exist(path):
        assets.delete_asset(path)
    options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    options.enable_collision = False
    options.enable_nanite = False
    mesh, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(whole, path, options)
    mesh.set_material(0, PROP_MATERIAL)
    assets.save_asset(path)
    size = box_of.max - box_of.min
    unreal.log_warning(f"PROP {name}: {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm, {outcome}")


def make_material():
    """Colour and shine come from each vertex, so a prop of wood and steel is still one mesh with one material."""
    path = f"{FOLDER}/M_Prop"
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_Prop", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    library = unreal.MaterialEditingLibrary
    vertex = library.create_material_expression(material, unreal.MaterialExpressionVertexColor, -600, 0)
    library.connect_material_property(vertex, "", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(vertex, "A", unreal.MaterialProperty.MP_METALLIC)

    rough = library.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -300, 200)
    rough.set_editor_property("const_a", 0.78)
    rough.set_editor_property("const_b", 0.3)
    library.connect_material_expressions(vertex, "A", rough, "Alpha")
    library.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # A golden prop shines by its own light. The game turns this up on the legendary ones.
    glow = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 400)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    lit = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 400)
    library.connect_material_expressions(vertex, "", lit, "A")
    library.connect_material_expressions(glow, "", lit, "B")
    library.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    library.recompile_material(material)
    assets.save_asset(path)
    return material


BAT = [(0, 0), (2.7, 0), (3.0, 1.2), (1.7, 3.5), (1.5, 26), (2.3, 46), (3.4, 66), (3.5, 77), (2.6, 81), (0, 81.5)]


def bat(name, wood, grip):
    save(name, [turned(wood, BAT), rod(grip, 1.75, 17, at(z=4))], lying=True)


def hammer(name, handle, head, long):
    pieces = [rod(handle, 1.9, long, at())]
    if name == "SM_Sledge":
        pieces.append(rod(head, 6.5, 20, at(x=-10, z=long - 5, pitch=-90)))
        pieces.append(rod(BLACK_IRON, 6.8, 2, at(x=-1, z=long - 5, pitch=-90)))
    else:
        pieces.append(box(head, (28, 13, 13), at(z=long - 4)))
        pieces.append(box(handle, (30, 9, 9), at(z=long - 4)))
    save(name, pieces, lying=True)


def melon():
    """Stripes need a colour for every vertex: they follow the longitude."""
    mesh = unreal.DynamicMesh()
    shapes.append_sphere_lat_long(mesh, unreal.GeometryScriptPrimitiveOptions(), at(scale=(1.0, 1.0, 0.84)), 18, 14, 24, CENTER)
    found = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(mesh, False)
    positions = next(item for item in found if isinstance(item, unreal.GeometryScriptVectorList))
    listed = unreal.GeometryScript_List.convert_vector_list_to_array(positions)
    shades = []
    for point in listed:
        around = math.atan2(point.y, point.x)
        shades.append(unreal.LinearColor(*(MELON_STRIPE if math.sin(around * 7.0) > 0.35 else MELON)))
    colour_list = unreal.GeometryScript_List.convert_array_to_color_list(shades)
    colours.set_mesh_per_vertex_colors(mesh, colour_list)
    save("SM_Melon", [mesh])


unreal.EditorAssetLibrary.make_directory(FOLDER)
PROP_MATERIAL = make_material()

save("SM_Crate", [
    box(WOOD, (38, 38, 38), at()),
    *[box(DARK_WOOD, (39.5, 39.5, 4.5), at(z=z)) for z in (-14.5, 14.5)],
    *[box(DARK_WOOD, (4.5, 4.5, 39.5), at(x=x, y=y)) for x in (-17.2, 17.2) for y in (-17.2, 17.2)],
    *[box(PALE_WOOD, (38.6, 1.2, 24), at(y=y)) for y in (-19, 19)],
])

save("SM_Bottle", [
    turned(GLASS, [(0, 0), (4.2, 0), (4.6, 1.2), (4.6, 15), (3.9, 19), (1.7, 23.5), (1.5, 28), (2.0, 28.6), (2.0, 30), (0, 30)]),
    turned(LABEL, [(4.75, 5), (4.75, 12.5)]),
])

try:
    melon()
except Exception as error:
    # The striped one needs calls that have changed names between engine versions. A plain one is better than none.
    unreal.log_warning(f"PROP SM_Melon: stripes failed ({error}); plain instead")
    save("SM_Melon", [ball(MELON, 18, at(scale=(1.0, 1.0, 0.84)))])

save("SM_Cone", [
    box(ORANGE, (32, 32, 3), at(z=1.5)),
    turned(ORANGE, [(0, 3), (11.5, 3), (3.3, 48), (2.6, 50), (0, 50)]),
    turned(WHITE, [(8.1, 23), (6.3, 33)]),
])

save("SM_Pan", [
    turned(BLACK_IRON, [(0, 0), (15, 0), (19, 5.2), (18, 5.2), (14.6, 1.2), (0, 1.2)], steps=24),
    box(BLACK_IRON, (10, 3, 1.6), at(x=23, z=4.4)),
    box(DARK_WOOD, (17, 3.6, 2.6), at(x=36, z=4.4)),
])

hammer("SM_GoldHammer", DEEP_GOLD, GOLD, 62)
bat("SM_GoldBat", GOLD, DEEP_GOLD)
save("SM_GoldFish", [
    ball(GOLD, 10, at(scale=(2.1, 0.62, 1.0))),
    taper(GOLD, 0.6, 9, 13, at(x=-17, pitch=90, scale=(1.0, 0.25, 1.0))),
    taper(DEEP_GOLD, 5, 0.4, 7, at(x=2, z=8, scale=(1.6, 0.2, 1.0))),
    ball(TAPE, 1.3, at(x=14.5, y=4.6, z=2.5)),
    ball(TAPE, 1.3, at(x=14.5, y=-4.6, z=2.5)),
])

bat("SM_WoodBat", PALE_WOOD, TAPE)
save("SM_IronBar", [
    rod(BLACK_IRON, 1.7, 92, at()),
    rod(BLACK_IRON, 1.7, 11, at(z=91, pitch=55)),
    rod(STEEL, 1.9, 3, at(z=20)),
], lying=True)
hammer("SM_Sledge", PALE_WOOD, STEEL, 78)
