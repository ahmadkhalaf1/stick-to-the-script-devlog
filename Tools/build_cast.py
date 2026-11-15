# Brings the cast into the project: one textured, skinned Mixamo character for the hero and one for each kind of
# opponent, from the FBX files of Saved/Incoming/Mixamo/Characters, named by role: Hero.fbx, Thug.fbx, Rusher.fbx,
# Kicker.fbx, Heavy.fbx, Gunman.fbx, Boss.fbx, and Kid.fbx for the captive. Each lands in Content/Characters/Cast/<Role> as SK_<Role> with its own
# materials and textures. The game poses them bone by bone on its sixteen joints, as it does the dummy, so they bring
# no animation. A role with no file keeps the dummy. Needs ffmpeg, to halve the pictures that come too wide.
#
# A role that is in the project is left as it is: bringing it in again makes every file of it new, a hundred megabytes
# of history for nothing. It is brought in afresh when its FBX file is newer than its model, which is how a character
# is replaced: put the new file in the old one's place. To change M_CastCloth, delete it and the hero's folder.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import os
import subprocess

import unreal

SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Incoming/Mixamo/Characters/"
SCRATCH = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_intermediate_dir())
OUT = "/Game/Characters/Cast"
ROLES = ["Hero", "Thug", "Rusher", "Kicker", "Heavy", "Gunman", "Boss", "Kid"]
# The bones the game places. Fighter/DummyBody.cpp names the same ones.
NEEDED = ["Hips", "Spine", "Spine1", "Spine2", "Neck", "Head",
          "LeftArm", "LeftForeArm", "LeftHand", "RightArm", "RightForeArm", "RightHand",
          "LeftUpLeg", "LeftLeg", "LeftFoot", "RightUpLeg", "RightLeg", "RightFoot"]
# A character's pictures come four thousand dots wide. Half that is more than the game shows of anyone.
WIDEST_PICTURE = 2048
# The hero's two pictures, for the wardrobe's dye. The box (left, top, right, bottom, as shares of the picture) is
# what is dark in it and is not cloth: his eyes, brows and hair. The number is how much of the wardrobe's colour its
# cloth takes. The colours are bright ones, made for a plain figure: in full, on cloth under the set's lights, they
# glowed. The hood and the cap take enough to be that colour, and the trousers a dark shade of it.
HERO_CLOTH = {
    "Ch06_1001_Diffuse": ((0.32, 0.0, 1.0, 0.39), 0.4),
    "Ch06_1002_Diffuse": ((0.0, 0.0, 0.0, 0.0), 0.1),
}

assets = unreal.EditorAssetLibrary


def said(text):
    unreal.log_warning(f"CAST {text}")


def shrink(picture, folder):
    """Makes a picture that is too wide half as wide, in the project itself and not only in the packaged game: four
    such pictures to a character were 60 MB of it in the repository. Written out, made smaller by ffmpeg, and brought
    back in under its own name, with what it was set to (a normal map stays one)."""
    name = picture.get_name()
    kept = {key: picture.get_editor_property(key) for key in ("compression_settings", "srgb", "lod_group")}
    wide = os.path.join(SCRATCH, name + "_wide.png")
    small = os.path.join(SCRATCH, name + ".png")
    out = unreal.AssetExportTask()
    out.object = picture
    out.filename = wide
    out.automated = True
    out.prompt = False
    out.replace_identical = True
    out.exporter = unreal.TextureExporterPNG()
    if not unreal.Exporter.run_asset_export_task(out) or not os.path.exists(wide):
        said(f"{name}: could not be written out, so it stays wide and is only shown smaller")
        picture.set_editor_property("max_texture_size", WIDEST_PICTURE)
        assets.save_loaded_asset(picture)
        return
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", wide, "-vf", f"scale={WIDEST_PICTURE}:{WIDEST_PICTURE}:flags=lanczos", small], check=True)
    back = unreal.AssetImportTask()
    back.filename = small
    back.destination_path = folder
    back.destination_name = name
    back.replace_existing = True
    back.automated = True
    back.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([back])
    again = unreal.load_asset(f"{folder}/{name}")
    for key, value in kept.items():
        again.set_editor_property(key, value)
    assets.save_loaded_asset(again)
    os.remove(wide)
    os.remove(small)


def cloth_material(colour, relief):
    """M_CastCloth: a character's own pictures, with what is dark and without colour in them swapped for a tint. The
    hero's hood, trousers and cap are black and plain, and his skin, shoes and headphones are not, so this dyes his
    clothes and nothing else. Tint's alpha is how much of the dye is taken: none leaves him as he was made."""
    library = unreal.MaterialEditingLibrary
    if assets.does_asset_exist(f"{OUT}/M_CastCloth"):
        return unreal.load_asset(f"{OUT}/M_CastCloth")
    cloth = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_CastCloth", OUT, unreal.Material, unreal.MaterialFactoryNew())
    cloth.set_editor_property("used_with_skeletal_mesh", True)
    seen = library.create_material_expression(cloth, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    seen.set_editor_property("parameter_name", "Diffuse")
    seen.set_editor_property("texture", colour)
    bumps = library.create_material_expression(cloth, unreal.MaterialExpressionTextureSampleParameter2D, -800, 320)
    bumps.set_editor_property("parameter_name", "Normal")
    bumps.set_editor_property("texture", relief)
    bumps.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    tint = library.create_material_expression(cloth, unreal.MaterialExpressionVectorParameter, -800, -260)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    skin = library.create_material_expression(cloth, unreal.MaterialExpressionVectorParameter, -800, -520)
    skin.set_editor_property("parameter_name", "Skin")
    skin.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    shade = library.create_material_expression(cloth, unreal.MaterialExpressionScalarParameter, -800, -700)
    shade.set_editor_property("parameter_name", "Shade")
    shade.set_editor_property("default_value", 1.0)
    where = library.create_material_expression(cloth, unreal.MaterialExpressionTextureCoordinate, -800, -820)
    mix = library.create_material_expression(cloth, unreal.MaterialExpressionCustom, -400, -60)
    mix.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    mix.set_editor_property("code",
        "float lum = dot(Colour, float3(0.3, 0.59, 0.11));\n"
        "float sat = max(Colour.r, max(Colour.g, Colour.b)) - min(Colour.r, min(Colour.g, Colour.b));\n"
        "float cloth = (1 - smoothstep(0.05, 0.13, lum)) * (1 - smoothstep(0.03, 0.08, sat));\n"
        "float skin = step(Skin.x, Where.x) * step(Where.x, Skin.z) * step(Skin.y, Where.y) * step(Where.y, SkinEnd);\n"
        # The black cloth is all within a hundredth of black, and that little is all the folds there are to keep.
        # The power deepens the tint: the light on the hero is strong, and washes a colour out.
        "return lerp(Colour, pow(Tint, 1.6) * Shade * saturate(0.35 + lum * 45), cloth * (1 - skin) * Strength);")
    wires = [("Colour", seen, "RGB"), ("Tint", tint, ""), ("Strength", tint, "A"), ("Skin", skin, ""), ("SkinEnd", skin, "A"),
             ("Shade", shade, ""), ("Where", where, "")]
    pins = []
    for name, _, _ in wires:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    mix.set_editor_property("inputs", pins)
    for name, source, output in wires:
        if not library.connect_material_expressions(source, output, mix, name):
            raise RuntimeError(f"M_CastCloth: nothing could be wired to {name}")
    rough = library.create_material_expression(cloth, unreal.MaterialExpressionConstant, -400, 200)
    rough.set_editor_property("r", 0.8)
    library.connect_material_property(mix, "", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(bumps, "RGB", unreal.MaterialProperty.MP_NORMAL)
    library.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.recompile_material(cloth)
    assets.save_loaded_asset(cloth)
    return cloth


def dye(mesh, folder):
    """Gives the hero clothes the wardrobe can dye: each of his opaque materials becomes an instance of M_CastCloth
    with the same two pictures. Run again, it finds them done."""
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    library = unreal.MaterialEditingLibrary
    # Each slot is a copy of what the model holds, so the changed ones are gathered and handed back whole.
    worn = []
    for slot in mesh.get_editor_property("materials"):
        old = slot.get_editor_property("material_interface")
        worn.append(slot)
        if not isinstance(old, unreal.MaterialInstanceConstant) or old.get_name().endswith("_Dyed") or old.get_blend_mode() != unreal.BlendMode.BLEND_OPAQUE:
            continue
        colour = library.get_material_instance_texture_parameter_value(old, "DiffuseColorMap")
        relief = library.get_material_instance_texture_parameter_value(old, "NormalMap")
        if not colour or not relief or colour.get_name() not in HERO_CLOTH:
            continue
        box, depth = HERO_CLOTH[colour.get_name()]
        made = tools.create_asset(f"MI_{old.get_name()}_Dyed", folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        library.set_material_instance_parent(made, cloth_material(colour, relief))
        library.set_material_instance_texture_parameter_value(made, "Diffuse", colour)
        library.set_material_instance_texture_parameter_value(made, "Normal", relief)
        library.set_material_instance_vector_parameter_value(made, "Skin", unreal.LinearColor(*box))
        library.set_material_instance_scalar_parameter_value(made, "Shade", depth)
        assets.save_loaded_asset(made)
        slot.set_editor_property("material_interface", made)
    mesh.set_editor_property("materials", worn)
    assets.save_loaded_asset(mesh)
    now = [slot.get_editor_property("material_interface").get_name() for slot in mesh.get_editor_property("materials")]
    if sum(name.endswith("_Dyed") for name in now) < len(HERO_CLOTH):
        raise RuntimeError(f"The hero's clothes cannot be dyed: his materials are {now}")
    said(f"dyed: the hero wears {now}")


made_roles = []
for role in ROLES:
    source = SOURCE + role + ".fbx"
    folder = f"{OUT}/{role}"
    model_file = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()) + f"Characters/Cast/{role}/SK_{role}.uasset"
    kept = os.path.exists(model_file) and not (os.path.exists(source) and os.path.getmtime(source) > os.path.getmtime(model_file))
    if not kept:
        if not os.path.exists(source):
            said(f"{role}: no {role}.fbx, so it keeps the dummy")
            continue
        if assets.does_directory_exist(folder):
            assets.delete_directory(folder)

        options = unreal.FbxImportUI()
        options.set_editor_property("import_materials", True)
        options.set_editor_property("import_textures", True)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("create_physics_asset", False)
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = folder
        task.destination_name = f"SK_{role}"
        task.replace_existing = True
        task.automated = True
        task.save = True
        task.options = options
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = None
    for path in assets.list_assets(folder, recursive=True):
        loaded = unreal.load_asset(path)
        if isinstance(loaded, unreal.SkeletalMesh):
            mesh = loaded
    if not mesh:
        raise RuntimeError(f"No skinned model came out of {source}")

    pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property("skeleton"))
    names = [str(name) for name in unreal.AnimPoseExtensions.get_bone_names(pose)]
    missing = [bone for bone in NEEDED if bone not in names]
    if missing:
        raise RuntimeError(f"{role}: the model has no bone called {missing}. It has {names[:30]}")
    hips = unreal.AnimPoseExtensions.get_bone_pose(pose, "Hips", unreal.AnimPoseSpaces.WORLD).translation
    head = unreal.AnimPoseExtensions.get_bone_pose(pose, "Head", unreal.AnimPoseSpaces.WORLD).translation
    foot = unreal.AnimPoseExtensions.get_bone_pose(pose, "LeftFoot", unreal.AnimPoseSpaces.WORLD).translation
    arm = unreal.AnimPoseExtensions.get_bone_pose(pose, "LeftArm", unreal.AnimPoseSpaces.WORLD).translation

    pictures = 0
    for path in assets.list_assets(folder, recursive=True):
        loaded = unreal.load_asset(path)
        if isinstance(loaded, unreal.Texture2D):
            pictures += 1
            if not kept and max(loaded.blueprint_get_size_x(), loaded.blueprint_get_size_y()) > WIDEST_PICTURE:
                shrink(loaded, folder)
        elif isinstance(loaded, unreal.Material) and not kept:
            # A material not marked for a skinned model is swapped for plain grey in the packaged game. In the
            # editor it would mark itself the first time it was drawn, and here nothing is drawn.
            if not loaded.get_editor_property("used_with_skeletal_mesh"):
                loaded.set_editor_property("used_with_skeletal_mesh", True)
                unreal.MaterialEditingLibrary.recompile_material(loaded)
            assets.save_loaded_asset(loaded)
    if not kept:
        assets.save_loaded_asset(mesh)
    if role == "Hero":
        dye(mesh, folder)

    slots = [str(slot.get_editor_property("material_slot_name")) for slot in mesh.get_editor_property("materials")]
    said(f"{role}: {'left as it is' if kept else 'brought in'}; {len(names)} bones, hips at {hips.z:.0f}, head at {head.z:.0f}, ankle at {foot.z:.0f}, "
         f"shoulder {abs(arm.y):.0f} or {abs(arm.x):.0f} out; {pictures} pictures; slots {slots}")
    made_roles.append(role)

said(f"done: {made_roles}")
unreal.SystemLibrary.quit_editor()
