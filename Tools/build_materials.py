# Builds the materials the sets and the figures are made of, in Content/Materials:
#   T_<Surface>_C and T_<Surface>_N   the textures painted by Tools/build_textures.mjs
#   M_Surface                          lays a texture on a block by its place in the world, so a stretched block does not stretch it
#   M_Figure                           what a stick figure is made of: smooth, with a light edge that lifts it off the set
#   M_Light                            a window, a bulb or a neon tube: one colour that gives light and is not shaded
#   M_Sign                             the same for lettering, cut to the shape of the letters
# All of it is generated, so this script and build_textures.mjs are the source. A material that exists is left alone:
# to change one, delete its .uasset and run this again.
#
# Run with the editor closed, after `node Tools/build_textures.mjs`:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout
# Then run the stage scripts again: their materials take their textures from here.

import os

import unreal

FOLDER = "/Game/Materials"
SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Textures"
SURFACES = ("Concrete", "Brick", "Planks", "Wallpaper", "Roofing", "Metal")

assets = unreal.EditorAssetLibrary
library = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# One texture is laid three ways, along each axis of the world, and each face takes the one that lies flat on it.
# On a wall the picture stands upright: its v runs down the wall.
LAY = """
float3 Place = Spot / Tile;
float3 Weight = pow(abs(Facing), 8.0);
Weight /= Weight.x + Weight.y + Weight.z;
float2 OnX = float2(Place.y, -Place.z);
float2 OnY = float2(Place.x, -Place.z);
float2 OnZ = float2(Place.x, Place.y);
"""

COLOUR_CODE = LAY + """
return Texture2DSample(Tex, TexSampler, OnX) * Weight.x
     + Texture2DSample(Tex, TexSampler, OnY) * Weight.y
     + Texture2DSample(Tex, TexSampler, OnZ) * Weight.z;
"""

# A normal map only keeps two numbers, so the third is worked out. Each of the three is then turned from the picture's
# own directions into the world's: along u, along v, and out of the face.
NORMAL_CODE = LAY + """
float2 A = (Texture2DSample(Tex, TexSampler, OnX).xy * 2.0 - 1.0) * Bump;
float2 B = (Texture2DSample(Tex, TexSampler, OnY).xy * 2.0 - 1.0) * Bump;
float2 C = (Texture2DSample(Tex, TexSampler, OnZ).xy * 2.0 - 1.0) * Bump;
float3 FromX = float3(sqrt(saturate(1.0 - dot(A, A))) * sign(Facing.x), A.x, -A.y);
float3 FromY = float3(B.x, sqrt(saturate(1.0 - dot(B, B))) * sign(Facing.y), -B.y);
float3 FromZ = float3(C.x, C.y, sqrt(saturate(1.0 - dot(C, C))) * sign(Facing.z));
return normalize(FromX * Weight.x + FromY * Weight.y + FromZ * Weight.z);
"""


def bring_in(name, is_normal):
    source = f"{SOURCE}/{name}.png"
    if not os.path.exists(source):
        raise RuntimeError(f"{source} is missing. Run `node Tools/build_textures.mjs` first.")
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = FOLDER
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    tools.import_asset_tasks([task])

    texture = unreal.load_asset(f"{FOLDER}/{name}")
    texture.set_editor_property("srgb", not is_normal)
    texture.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_NORMALMAP if is_normal else unreal.TextureCompressionSettings.TC_DEFAULT)
    assets.save_asset(f"{FOLDER}/{name}")
    return texture


def started(name):
    """A new, empty material by that name, or nothing when it is there already.
    To build one again, delete its file first: clearing a material from a script crashes this engine version."""
    if assets.does_asset_exist(f"{FOLDER}/{name}"):
        unreal.log_warning(f"MATERIALS kept {name} as it is")
        return None
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def node(material, kind, x, y, **properties):
    made = library.create_material_expression(material, kind, x, y)
    for name, value in properties.items():
        made.set_editor_property(name, value)
    return made


def custom(material, code, output, inputs, x, y):
    made = node(material, unreal.MaterialExpressionCustom, x, y, code=code, output_type=output)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    made.set_editor_property("inputs", pins)
    for name, source in inputs.items():
        if not library.connect_material_expressions(source, "", made, name):
            raise RuntimeError(f"Could not connect {name}")
    return made


def make_surface(colour_texture, normal_texture):
    material = started("M_Surface")
    if not material:
        return
    # The normal comes out in world directions, because that is where the three layings are put together.
    material.set_editor_property("tangent_space_normal", False)

    place = node(material, unreal.MaterialExpressionWorldPosition, -1100, 0)
    facing = node(material, unreal.MaterialExpressionVertexNormalWS, -1100, 150)
    tile = node(material, unreal.MaterialExpressionScalarParameter, -1100, 300, parameter_name="TileSize", default_value=200.0)
    bump = node(material, unreal.MaterialExpressionScalarParameter, -1100, 450, parameter_name="Bump", default_value=1.0)
    tint = node(material, unreal.MaterialExpressionVectorParameter, -500, -250, parameter_name="Tint", default_value=unreal.LinearColor(1, 1, 1, 1))
    colour = node(material, unreal.MaterialExpressionTextureObjectParameter, -1100, -250, parameter_name="Colour", texture=colour_texture,
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    normal = node(material, unreal.MaterialExpressionTextureObjectParameter, -1100, 650, parameter_name="Normal", texture=normal_texture,
                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)

    shared = {"Spot": place, "Facing": facing, "Tile": tile}
    laid = custom(material, COLOUR_CODE, unreal.CustomMaterialOutputType.CMOT_FLOAT4, {"Tex": colour, **shared}, -750, 0)
    relief = custom(material, NORMAL_CODE, unreal.CustomMaterialOutputType.CMOT_FLOAT3, {"Tex": normal, "Bump": bump, **shared}, -750, 450)

    shade = node(material, unreal.MaterialExpressionComponentMask, -450, 0, r=True, g=True, b=True, a=False)
    rough = node(material, unreal.MaterialExpressionComponentMask, -450, 150, r=False, g=False, b=False, a=True)
    library.connect_material_expressions(laid, "", shade, "")
    library.connect_material_expressions(laid, "", rough, "")
    tinted = node(material, unreal.MaterialExpressionMultiply, -200, -100)
    library.connect_material_expressions(shade, "", tinted, "A")
    library.connect_material_expressions(tint, "", tinted, "B")

    library.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.connect_material_property(relief, "", unreal.MaterialProperty.MP_NORMAL)
    library.recompile_material(material)
    assets.save_asset(f"{FOLDER}/M_Surface")


def make_figure():
    material = started("M_Figure")
    if not material:
        return
    # The game sets "Color" on every part of a figure, as it did on the plain material this replaces.
    colour = node(material, unreal.MaterialExpressionVectorParameter, -700, 0, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))
    library.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Smooth enough to catch the lamps as a highlight. A lacquer coat would do it better, but its pins are closed to scripts.
    library.connect_material_property(node(material, unreal.MaterialExpressionConstant, -300, 150, r=0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)

    # The edge that faces away from the camera glows a little in the figure's own colour, and a dark figure gets a grey one,
    # so every figure has an outline of light against a dark set.
    edge = node(material, unreal.MaterialExpressionFresnel, -700, 450, exponent=3.5, base_reflect_fraction=0.0)
    lifted = node(material, unreal.MaterialExpressionAdd, -500, 300, const_b=0.06)
    library.connect_material_expressions(colour, "", lifted, "A")
    glow = node(material, unreal.MaterialExpressionMultiply, -300, 450)
    library.connect_material_expressions(lifted, "", glow, "A")
    library.connect_material_expressions(edge, "", glow, "B")
    strength = node(material, unreal.MaterialExpressionScalarParameter, -500, 600, parameter_name="Edge", default_value=0.9)
    lit = node(material, unreal.MaterialExpressionMultiply, -100, 500)
    library.connect_material_expressions(glow, "", lit, "A")
    library.connect_material_expressions(strength, "", lit, "B")
    library.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    library.recompile_material(material)
    assets.save_asset(f"{FOLDER}/M_Figure")


def make_light():
    material = started("M_Light")
    if not material:
        return
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    colour = node(material, unreal.MaterialExpressionVectorParameter, -500, 0, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))
    strength = node(material, unreal.MaterialExpressionScalarParameter, -500, 200, parameter_name="Strength", default_value=6.0)
    lit = node(material, unreal.MaterialExpressionMultiply, -250, 100)
    library.connect_material_expressions(colour, "", lit, "A")
    library.connect_material_expressions(strength, "", lit, "B")
    library.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    library.recompile_material(material)
    assets.save_asset(f"{FOLDER}/M_Light")


def make_sign():
    material = started("M_Sign")
    if not material:
        return
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    colour = node(material, unreal.MaterialExpressionVectorParameter, -500, 0, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))
    strength = node(material, unreal.MaterialExpressionScalarParameter, -500, 200, parameter_name="Strength", default_value=15.0)
    lit = node(material, unreal.MaterialExpressionMultiply, -250, 100)
    library.connect_material_expressions(colour, "", lit, "A")
    library.connect_material_expressions(strength, "", lit, "B")
    library.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # The lettering the engine ships: a picture of how far each point is from the edge of a letter, cut at the half-way value.
    letters = node(material, unreal.MaterialExpressionFontSampleParameter, -500, 400, parameter_name="Font",
                   font=unreal.load_asset("/Engine/EngineFonts/RobotoDistanceField"))
    # That picture has one channel, which arrives as red.
    cut = node(material, unreal.MaterialExpressionComponentMask, -250, 400, r=True, g=False, b=False, a=False)
    library.connect_material_expressions(letters, "", cut, "")
    library.connect_material_property(cut, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    material.set_editor_property("opacity_mask_clip_value", 0.5)
    library.recompile_material(material)
    assets.save_asset(f"{FOLDER}/M_Sign")


assets.make_directory(FOLDER)
textures = {name: (bring_in(f"T_{name}_C", False), bring_in(f"T_{name}_N", True)) for name in SURFACES}
make_surface(*textures["Concrete"])
make_figure()
make_light()
make_sign()
unreal.log_warning(f"MATERIALS built: {len(textures)} surfaces, M_Surface, M_Figure, M_Light, M_Sign")
unreal.SystemLibrary.quit_editor()
