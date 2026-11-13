# Brings the fight clips of Saved/Incoming/Mixamo into the project and fits them to the mannequin's skeleton.
# The clips are Mixamo's (Adobe), downloaded as FBX with the X Bot's skin. What the game uses is the converted copy
# in Content/Characters/Fight, one animation per clip, named A_<Clip>.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import glob
import os

import unreal

SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Incoming/Mixamo"
RAW = "/Game/Characters/Fight/Mixamo"
OUT = "/Game/Characters/Fight"
MANNEQUIN = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def said(text):
    unreal.log_warning(f"ANIM {text}")


def clean(name):
    return "".join(part.capitalize() for part in name.replace("-", " ").split())


def bring_in(path, skeleton):
    """One FBX. The first is taken whole, mesh and all, to get Mixamo's skeleton; the rest only add their animation to it."""
    name = clean(os.path.splitext(os.path.basename(path))[0])
    options = unreal.FbxImportUI()
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("automated_import_should_detect_type", False)
    if skeleton:
        options.set_editor_property("import_mesh", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        options.set_editor_property("skeleton", skeleton)
    else:
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = RAW
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    task.options = options
    tools.import_asset_tasks([task])
    return name, [str(made) for made in task.get_editor_property("imported_object_paths")]


def rig_for(name, mesh):
    """An IK rig that knows which bones are the spine, the arms and the legs. The engine recognises both skeletons."""
    path = f"{RAW}/{name}"
    rig = tools.create_asset(name, RAW, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(mesh)
    controller.apply_auto_generated_retarget_definition()
    said(f"{name}: chains {[str(chain.chain_name) for chain in controller.get_retarget_chains()]}, root {controller.get_retarget_root()}")
    assets.save_asset(path)
    return rig


def travel(clip):
    """How far the busiest hand or foot gets from where the clip starts. A conversion that copied nothing gives 0."""
    options = unreal.AnimPoseEvaluationOptions()

    def places(seconds):
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, seconds, options)
        return [unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
                for bone in ("hand_l", "hand_r", "foot_l", "foot_r")]

    first = places(0.0)
    steps = int(clip.get_play_length() * 10)
    return max((now - then).length() for step in range(1, steps + 1) for now, then in zip(places(step / 10.0), first))


# Everything is made again from the FBX files. What an earlier run left is cleared first, all of it at once: the rigs
# and the retargeter refer to each other, and deleting one of them alone leaves a stub that blocks its replacement.
if assets.does_directory_exist(RAW):
    assets.delete_directory(RAW)
for old in assets.list_assets(OUT, recursive=False):
    if old.split("/")[-1].startswith("A_"):
        assets.delete_asset(old.split(".")[0])

files = sorted(glob.glob(f"{SOURCE}/*.fbx"))
if not files:
    raise RuntimeError(f"No FBX files in {SOURCE}")

skeleton = None
mixamo_mesh = None
clips = []
for path in files:
    name, made = bring_in(path, skeleton)
    for made_path in made:
        loaded = unreal.load_asset(made_path)
        if isinstance(loaded, unreal.SkeletalMesh) and not mixamo_mesh:
            mixamo_mesh = loaded
            skeleton = loaded.get_editor_property("skeleton")
        elif isinstance(loaded, unreal.AnimSequence):
            clips.append(loaded)
    said(f"imported {name}: {[made_path.split('.')[-1] for made_path in made]}")
if not mixamo_mesh or not clips:
    raise RuntimeError(f"Import gave no mesh or no animations (mesh {mixamo_mesh}, clips {len(clips)})")

mannequin = unreal.load_asset(MANNEQUIN)
source_rig = rig_for("IK_Mixamo", mixamo_mesh)
target_rig = rig_for("IK_Mannequin", mannequin)

retargeter_path = f"{RAW}/RTG_MixamoToMannequin"
retargeter = tools.create_asset("RTG_MixamoToMannequin", RAW, unreal.IKRetargeter, unreal.IKRetargetFactory())
fitting = unreal.IKRetargeterController.get_controller(retargeter)
fitting.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
fitting.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
# A retargeter made from a script starts with no steps at all, and then copies nothing: every clip comes out standing still.
fitting.add_default_ops()
fitting.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
# Mixamo's figure stands with its arms straight out and the mannequin with them lowered: each is turned to match the other.
fitting.auto_align_all_bones(unreal.RetargetSourceOrTarget.SOURCE)
assets.save_asset(retargeter_path)

converted = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    [unreal.AssetRegistryHelpers.create_asset_data(clip) for clip in clips], mixamo_mesh, mannequin, retargeter,
    prefix="A_", target_path=OUT, include_referenced_assets=False)
still = []
for made in converted:
    asset = made.get_asset()
    wanted = f"{OUT}/{asset.get_name()}"
    if str(made.package_name) != wanted:
        if assets.does_asset_exist(wanted):
            assets.delete_asset(wanted)
        assets.rename_asset(str(made.package_name), wanted)
    assets.save_asset(wanted)
    clip = unreal.load_asset(wanted)
    moved = travel(clip)
    said(f"converted {clip.get_name()}: {clip.get_play_length():.2f} s, a hand or foot travels {moved:.0f} cm")
    if moved < 0.5:
        still.append(clip.get_name())
said(f"done: {len(converted)} of {len(clips)} clips converted, {fitting.get_num_retarget_ops()} retarget steps")
if still:
    raise RuntimeError(f"Converted, but nothing in them moves: {still}")
unreal.SystemLibrary.quit_editor()
