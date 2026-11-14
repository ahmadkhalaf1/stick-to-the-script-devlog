# Brings the scanned models of Saved/Incoming/PolyHaven into the project and makes the game's props from them.
# The models are Poly Haven's (polyhaven.com), all CC0; Tools/fetch_scans.mjs downloads them.
#
# For each model: its pictures become textures in Content/Scans/<id>, a material is made for them, and its FBX is taken
# apart into its pieces. A prop is then put together from the pieces named in PROPS, turned so that it lies along X the
# way the game holds things, sized, re-centred and saved into Content/Props under the name the game looks for.
#
# The same models also dress the sets: SET_PIECES lists what a stage script can put down with dress().
#
# Run with the editor closed, after Tools/build_props.py (the golden props wear its material):
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -EnablePlugins=GeometryScripting -unattended -nosplash -nullrhi -stdout

import glob
import os

import unreal

SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Incoming/PolyHaven"
SCANS = "/Game/Scans"
PROPS = "/Game/Props"
MATERIALS = "/Game/Materials"
# The props' own material reads colour and shine from each corner of a mesh: red, green, blue, and how much of a metal.
GOLD = (1.0, 0.72, 0.16, 1.0)
BOTTLE_GLASS = (0.02, 0.16, 0.05, 0.75)

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.MaterialEditingLibrary
shapes = unreal.GeometryScript_MeshTransforms


def said(text):
    unreal.log_warning(f"SCAN {text}")


# One row for each prop: the name the game loads, the model it is made from, the pieces of that model to use (each a
# part of a piece's name; none means all of them), how to turn it so its long way lies along X, and how much to scale it.
# A painted prop keeps the model's shape and wears the props' own material in one colour: the golden ones, and the bottle,
# whose scanned glass is see-through and has no pictures of its own.
PROPS_TO_MAKE = [
    {"name": "SM_Pistol", "model": "service_pistol", "pieces": ["pistol_a", "slide_a", "hammer_a", "trigger_a"]},
    {"name": "SM_Rifle", "model": "bolt_action_rifle_7_62", "pieces": ["rifle_7_62", "bolt_a", "trigger", "scope", "wrap"], "not": ["bullet", "bolt_b"]},
    {"name": "SM_WoodBat", "model": "baseball_bat"},
    {"name": "SM_GoldBat", "model": "baseball_bat", "paint": GOLD},
    {"name": "SM_IronBar", "model": "crowbar_01", "scale": 1.4},
    {"name": "SM_Sledge", "model": "sledgehammer_01"},
    {"name": "SM_GoldHammer", "model": "ornate_war_hammer", "paint": GOLD},
    {"name": "SM_GoldFish", "model": "antique_katana_01", "paint": GOLD},
    {"name": "SM_Crate", "model": "wooden_crate_02", "scale": 0.5},
    {"name": "SM_Pan", "model": "brass_pan_01"},
    {"name": "SM_Cone", "model": "WetFloorSign_01", "upright": True},
    {"name": "SM_Melon", "model": "bar_chair_round_01", "upright": True, "scale": 0.8},
    {"name": "SM_Bottle", "model": "wine_bottles_01", "pieces": ["bordeaux"], "upright": True, "paint": BOTTLE_GLASS},
]


# Things that only dress a set, each from a whole model unless it names pieces. It is left standing as it was modelled
# and saved as SM_Set<name>, with the floor under its middle, so a stage script puts it down by the point it stands on.
# One that hangs is held by its top. "tall" sizes it to that many centimetres; "turn" lays it over first.
SET_PIECES = [
    {"name": "DrumRed", "model": "Barrel_01"},
    {"name": "DrumBlue", "model": "barrel_03"},
    {"name": "ArmyCrate", "model": "old_military_crate", "not": ["_b"]},
    {"name": "LongCrate", "model": "wooden_military_crate"},
    {"name": "Crate", "model": "wooden_crate_02"},
    {"name": "Carton", "model": "cardboard_box_01"},
    {"name": "Shelves", "model": "steel_frame_shelves_01", "tall": 214},
    {"name": "Tyre", "model": "old_tyre", "turn": (90.0, 0.0, 0.0)},
    {"name": "ToolChest", "model": "metal_tool_chest"},
    {"name": "Extinguisher", "model": "korean_fire_extinguisher_01"},
    {"name": "CabinetGreen", "model": "utility_box_02"},
    {"name": "CabinetBlue", "model": "utility_box_01"},
    {"name": "FuseBox", "model": "power_box_01"},
    {"name": "HangingLamp", "model": "hanging_industrial_lamp", "hang": True},
    {"name": "Generator", "model": "portable_generator"},
    {"name": "PlasticCrate", "model": "plastic_crate_02"},
    {"name": "Table", "model": "round_wooden_table_01", "tall": 78},
    {"name": "Chair", "model": "GreenChair_01"},
    {"name": "BackBar", "model": "vintage_cabinet_01"},
    {"name": "Fan", "model": "ceiling_fan", "hang": True},
    {"name": "Television", "model": "television_02"},
    {"name": "Chalkboard", "model": "standing_chalkboard_01"},
    {"name": "Cask", "model": "wine_barrel_01"},
    {"name": "Stool", "model": "bar_chair_round_01"},
    {"name": "AirUnits", "model": "exterior_aircon_unit"},
    {"name": "GasBottle", "model": "propane_tank"},
    {"name": "Camera", "model": "security_camera_01"},
    {"name": "WallLamp", "model": "industrial_wall_lamp"},
    {"name": "Bin", "model": "metal_trash_can", "pieces": ["rust"]},
    {"name": "Barrier", "model": "concrete_road_barrier"},
]


def picture(path, name, folder):
    """One texture. What kind it is shows in its file name, and decides how the engine has to read it."""
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(f"{folder}/{name}")
    if "_nor_gl_" in path:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
        # Poly Haven's bumps point the other way up from the engine's.
        texture.set_editor_property("flip_green_channel", True)
    elif "_arm_" in path:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        texture.set_editor_property("srgb", False)
    assets.save_asset(f"{folder}/{name}")
    return texture


def scan_material(colour, normal, mix):
    """The one material every scan wears: its colour, its bumps, and a third picture whose red is how much light reaches
    into its creases, green how rough it is and blue how much of a metal."""
    path = f"{MATERIALS}/M_Scan"
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    material = tools.create_asset("M_Scan", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())

    def sample(name, texture, kind, y):
        node = library.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -600, y)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("texture", texture)
        node.set_editor_property("sampler_type", kind)
        return node

    library.connect_material_property(sample("Colour", colour, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -300), "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(sample("Normal", normal, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 0), "RGB", unreal.MaterialProperty.MP_NORMAL)
    packed = sample("Mix", mix, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 300)
    library.connect_material_property(packed, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    library.connect_material_property(packed, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    library.connect_material_property(packed, "B", unreal.MaterialProperty.MP_METALLIC)
    library.recompile_material(material)
    assets.save_asset(path)
    return material


def bring_in(model):
    """A model's textures, a material for each set of them, and its pieces. Returns the pieces and the materials by name."""
    folder = f"{SCANS}/{model}"
    if assets.does_directory_exist(folder):
        assets.delete_directory(folder)

    # A model with several materials has a set of pictures for each, told apart by what comes before _diff_ in the name.
    materials = {}
    for colour_path in sorted(glob.glob(f"{SOURCE}/{model}/textures/*_diff_*.jpg")):
        stem = os.path.basename(colour_path).split("_diff_")[0]
        normal_path = colour_path.replace("_diff_", "_nor_gl_")
        mix_path = colour_path.replace("_diff_", "_arm_")
        if not os.path.exists(normal_path) or not os.path.exists(mix_path):
            said(f"{model}: {stem} has no bumps or no mix picture, and is left out")
            continue
        colour = picture(colour_path, f"T_{stem}_Colour", folder)
        normal = picture(normal_path, f"T_{stem}_Normal", folder)
        mix = picture(mix_path, f"T_{stem}_Mix", folder)
        parent = scan_material(colour, normal, mix)
        instance = tools.create_asset(f"MI_{stem}", folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        library.set_material_instance_parent(instance, parent)
        library.set_material_instance_texture_parameter_value(instance, "Colour", colour)
        library.set_material_instance_texture_parameter_value(instance, "Normal", normal)
        library.set_material_instance_texture_parameter_value(instance, "Mix", mix)
        assets.save_asset(f"{folder}/MI_{stem}")
        materials[stem] = instance

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    how = options.get_editor_property("static_mesh_import_data")
    how.set_editor_property("combine_meshes", False)
    how.set_editor_property("auto_generate_collision", False)
    how.set_editor_property("generate_lightmap_u_vs", False)
    task = unreal.AssetImportTask()
    task.filename = f"{SOURCE}/{model}/{model}.fbx"
    task.destination_path = f"{folder}/Pieces"
    task.replace_existing = True
    task.automated = True
    task.save = True
    task.options = options
    tools.import_asset_tasks([task])
    pieces = [unreal.load_asset(str(path)) for path in task.get_editor_property("imported_object_paths")]
    pieces = [piece for piece in pieces if isinstance(piece, unreal.StaticMesh)]
    for piece in pieces:
        box = piece.get_bounding_box()
        slots = [str(slot.material_slot_name) for slot in piece.static_materials]
        said(f"{model} piece {piece.get_name()}: ({box.min.x:.0f}, {box.min.y:.0f}, {box.min.z:.0f}) to ({box.max.x:.0f}, {box.max.y:.0f}, {box.max.z:.0f}), slots {slots}")
    return pieces, materials


def material_for(slot_name, materials):
    """The material whose pictures were named for this slot: the longest name that the slot's name contains."""
    fits = [stem for stem in materials if stem.lower() in slot_name.lower() or slot_name.lower() in stem.lower()]
    return max(fits, key=len) if fits else sorted(materials, key=len)[0]


def make(row, pieces, materials):
    wanted = row.get("pieces")
    unwanted = row.get("not", [])
    chosen = [piece for piece in pieces
              if (not wanted or any(part in piece.get_name() for part in wanted)) and not any(part in piece.get_name() for part in unwanted)]
    if not chosen:
        raise RuntimeError(f"{row['name']}: none of {wanted} among {[piece.get_name() for piece in pieces]}")

    # Every piece counts its materials from nothing. Put together, each has to point at its own in one shared list.
    order = sorted(materials)
    whole = unreal.DynamicMesh()
    for piece in chosen:
        part = unreal.DynamicMesh()
        unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(piece, part, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
        slots = [str(slot.material_slot_name) for slot in piece.static_materials]
        for index in range(len(slots)):
            unreal.GeometryScript_Materials.remap_material_i_ds(part, index, 100 + index)
        for index, slot in enumerate(slots):
            unreal.GeometryScript_Materials.remap_material_i_ds(part, 100 + index, order.index(material_for(slot, materials)))
        unreal.GeometryScript_MeshEdits.append_mesh(whole, part, unreal.Transform())

    # Longest way along X, which is how the game holds and aims things. Something that stands on the floor is left standing.
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(whole)
    size = box.max - box.min
    if not row.get("upright"):
        if size.y >= size.x and size.y >= size.z:
            shapes.rotate_mesh(whole, unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), unreal.Vector(0, 0, 0))
        elif size.z >= size.x and size.z >= size.y:
            shapes.rotate_mesh(whole, unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0), unreal.Vector(0, 0, 0))
    turn = row.get("turn")
    if turn:
        shapes.rotate_mesh(whole, unreal.Rotator(roll=turn[0], pitch=turn[1], yaw=turn[2]), unreal.Vector(0, 0, 0))
    scale = row.get("scale", 1.0)
    if row.get("tall"):
        box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(whole)
        scale = row["tall"] / (box.max.z - box.min.z)
    if scale != 1.0:
        shapes.scale_mesh(whole, unreal.Vector(scale, scale, scale), unreal.Vector(0, 0, 0), True)
    if row.get("paint"):
        unreal.GeometryScript_Materials.clear_material_i_ds(whole, 0)
        unreal.GeometryScript_VertexColors.set_mesh_constant_vertex_color(whole, unreal.LinearColor(*row["paint"]), unreal.GeometryScriptColorFlags(), True)
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(whole)
    middle = (box.min + box.max) * 0.5
    rest = box.min.z if row.get("stand") else box.max.z if row.get("hang") else middle.z
    shapes.translate_mesh(whole, unreal.Vector(-middle.x, -middle.y, -rest))

    path = f"{PROPS}/{row['name']}"
    if assets.does_asset_exist(path):
        assets.delete_asset(path)
    options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    options.enable_collision = False
    options.enable_nanite = False
    options.enable_recompute_normals = False
    mesh, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(whole, path, options)
    wearing = [unreal.load_asset(f"{PROPS}/M_Prop")] if row.get("paint") else [materials[stem] for stem in order]
    for index, material in enumerate(wearing):
        if index < len(mesh.static_materials):
            mesh.set_material(index, material)
        else:
            mesh.add_material(material)
    assets.save_asset(path)
    size = box.max - box.min
    said(f"made {row['name']} from {row['model']}: {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm, {len(chosen)} pieces, {len(mesh.static_materials)} materials, {outcome}")


brought = {}
set_rows = [dict(row, name=f"SM_Set{row['name']}", upright=True, stand=not row.get("hang")) for row in SET_PIECES]
for row in PROPS_TO_MAKE + set_rows:
    if row["model"] not in brought:
        brought[row["model"]] = bring_in(row["model"])
    make(row, *brought[row["model"]])
said(f"done: {len(PROPS_TO_MAKE)} props and {len(set_rows)} set pieces from {len(brought)} scans")
unreal.SystemLibrary.quit_editor()
