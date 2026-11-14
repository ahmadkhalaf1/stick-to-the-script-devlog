# Brings the fighters' body into the project: the skinned model that comes with the Mixamo clips (Adobe's X Bot),
# taken from one of the FBX files of Saved/Incoming/Mixamo. The game poses it bone by bone on its sixteen joints,
# so it brings no animation and no material of its own. It lands in Content/Characters/Dummy as SK_Dummy.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import unreal

SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Incoming/Mixamo/Fighting Idle.fbx"
OUT = "/Game/Characters/Dummy"
NAME = "SK_Dummy"
# The bones the game places. Fighter/StickFighter.cpp names the same ones.
NEEDED = ["Hips", "Spine", "Spine1", "Spine2", "Neck", "Head",
          "LeftArm", "LeftForeArm", "LeftHand", "RightArm", "RightForeArm", "RightHand",
          "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase"]

assets = unreal.EditorAssetLibrary


def said(text):
    unreal.log_warning(f"DUMMY {text}")


if assets.does_directory_exist(OUT):
    assets.delete_directory(OUT)

options = unreal.FbxImportUI()
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_mesh", True)
options.set_editor_property("create_physics_asset", False)
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = OUT
task.destination_name = NAME
task.replace_existing = True
task.automated = True
task.save = True
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = None
for made in task.get_editor_property("imported_object_paths"):
    loaded = unreal.load_asset(str(made))
    said(f"made {str(made).split('.')[-1]}: {type(loaded).__name__}")
    if isinstance(loaded, unreal.SkeletalMesh):
        mesh = loaded
if not mesh:
    raise RuntimeError(f"No skinned model came out of {SOURCE}")

pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property("skeleton"))
names = [str(name) for name in unreal.AnimPoseExtensions.get_bone_names(pose)]
missing = [bone for bone in NEEDED if bone not in names]
if missing:
    raise RuntimeError(f"The model has no bone called {missing}. It has {names[:30]}")
for bone in NEEDED:
    place = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
    said(f"{bone}: at ({place.translation.x:.0f}, {place.translation.y:.0f}, {place.translation.z:.0f})")
# The model wears the figures' material, which was made for plain shapes. A material not marked for a skinned model
# is swapped for plain grey in the game, so it is marked here, where the need for it comes from.
figure = unreal.load_asset("/Game/Materials/M_Figure")
if not figure:
    raise RuntimeError("No /Game/Materials/M_Figure. Run Tools/build_materials.py first.")
if not figure.get_editor_property("used_with_skeletal_mesh"):
    figure.set_editor_property("used_with_skeletal_mesh", True)
    unreal.MaterialEditingLibrary.recompile_material(figure)
    assets.save_asset("/Game/Materials/M_Figure")
    said("M_Figure is now allowed on a skinned model")
said(f"slots: {[str(slot.get_editor_property('material_slot_name')) for slot in mesh.get_editor_property('materials')]}")
said(f"done: {mesh.get_path_name()}, {len(names)} bones")
unreal.SystemLibrary.quit_editor()
