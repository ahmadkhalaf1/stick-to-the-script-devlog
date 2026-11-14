# Brings the menu's scene cards into Content/UI: Tools/ui/Scene<n>.png becomes T_SceneCard<n>, and Scene<n>_Locked.png,
# the same picture out of focus, becomes T_SceneCard<n>_Locked. UI/StickHUD.cpp draws them.
# The pictures are frames of the gameplay video: a 1504 by 846 piece from the middle of a film frame, scaled to 640 by 360,
# and for the locked one blurred and drained of colour (ffmpeg: gblur=sigma=11,eq=saturation=0.45:brightness=-0.1).
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import glob
import os

import unreal

SOURCE = unreal.Paths.project_dir() + "Tools/ui"
FOLDER = "/Game/UI"

made = []
for source in sorted(glob.glob(f"{SOURCE}/Scene*.png")):
    name = "T_SceneCard" + os.path.splitext(os.path.basename(source))[0][len("Scene"):]
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = FOLDER
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    picture = unreal.load_asset(f"{FOLDER}/{name}")
    if not picture:
        raise RuntimeError(f"{source} did not come in as {name}")
    # Drawn flat on the screen at its own size: no smaller copies, nothing streamed in late, no block compression.
    picture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    picture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    picture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    picture.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(picture)
    made.append(name)

if len(made) == 0 or len(made) % 2:
    raise RuntimeError(f"expected a picture and a locked picture for every scene in {SOURCE}, found {len(made)} files")
print(f"UI built with {len(made)} scene cards: {', '.join(made)}")
