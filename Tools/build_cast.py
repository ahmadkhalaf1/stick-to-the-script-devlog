# Brings the cast into the project: one textured, skinned Mixamo character for the hero and one for each kind of
# opponent, from the FBX files of Saved/Incoming/Mixamo/Characters, named by role: Hero.fbx, Thug.fbx, Rusher.fbx,
# Kicker.fbx, Heavy.fbx, Gunman.fbx, Boss.fbx. Each lands in Content/Characters/Cast/<Role> as SK_<Role> with its own
# materials and textures. The game poses them bone by bone on its sixteen joints, as it does the dummy, so they bring
# no animation. A role with no file keeps the dummy. Needs ffmpeg, to halve the pictures that come too wide.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import os
import subprocess

import unreal

SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Incoming/Mixamo/Characters/"
SCRATCH = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_intermediate_dir())
OUT = "/Game/Characters/Cast"
ROLES = ["Hero", "Thug", "Rusher", "Kicker", "Heavy", "Gunman", "Boss"]
# The bones the game places. Fighter/DummyBody.cpp names the same ones.
NEEDED = ["Hips", "Spine", "Spine1", "Spine2", "Neck", "Head",
          "LeftArm", "LeftForeArm", "LeftHand", "RightArm", "RightForeArm", "RightHand",
          "LeftUpLeg", "LeftLeg", "LeftFoot", "RightUpLeg", "RightLeg", "RightFoot"]
# A character's pictures come four thousand dots wide. Half that is more than the game shows of anyone.
WIDEST_PICTURE = 2048

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


made_roles = []
for role in ROLES:
    source = SOURCE + role + ".fbx"
    if not os.path.exists(source):
        said(f"{role}: no {role}.fbx, so it keeps the dummy")
        continue
    folder = f"{OUT}/{role}"
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
    materials = []
    for path in assets.list_assets(folder, recursive=True):
        loaded = unreal.load_asset(path)
        if isinstance(loaded, unreal.Texture2D):
            pictures += 1
            if max(loaded.blueprint_get_size_x(), loaded.blueprint_get_size_y()) > WIDEST_PICTURE:
                shrink(loaded, folder)
        elif isinstance(loaded, unreal.Material):
            # A material not marked for a skinned model is swapped for plain grey in the packaged game. In the
            # editor it would mark itself the first time it was drawn, and here nothing is drawn.
            if not loaded.get_editor_property("used_with_skeletal_mesh"):
                loaded.set_editor_property("used_with_skeletal_mesh", True)
                unreal.MaterialEditingLibrary.recompile_material(loaded)
            assets.save_loaded_asset(loaded)
            materials.append(f"{loaded.get_name()}:{str(loaded.get_editor_property('blend_mode')).split('.')[-1]}")
    assets.save_loaded_asset(mesh)

    slots = [str(slot.get_editor_property("material_slot_name")) for slot in mesh.get_editor_property("materials")]
    said(f"{role}: {len(names)} bones, hips at {hips.z:.0f}, head at {head.z:.0f}, ankle at {foot.z:.0f}, shoulder {abs(arm.y):.0f} or {abs(arm.x):.0f} out; "
         f"{pictures} pictures; slots {slots}; materials {materials}")
    made_roles.append(role)

said(f"done: {made_roles}")
unreal.SystemLibrary.quit_editor()
