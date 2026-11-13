# Brings every WAV in Saved/Audio into Content/Audio, where the game looks for recordings by name:
#   S_<Sound>_<take>   a sound the game otherwise makes in code. <Sound> is one of EImpactSound in UI/ImpactSynth.h
#                      (Punch, HeavyHit, Knockout, Whoosh ...), or Action, Cut or Wrap for the director calling them.
#                      Takes are numbered from 1 with no gap, six at most. One is picked at random each time.
#   V_Scene<n>_<line>  what the director shouts in scene n: the lines of "DirectorLines" in Content/Scripts/Scene0<n>.json, in order
#   M_Fight_<n>        the fight music of scene n (M_Fight, with no number, serves a scene that has none of its own)
#   M_Film             the music of the film. Music loops.
# A sound with no file keeps being made in code, so any part of this can be missing.
#
# The files come from `node Tools/build_sounds.mjs --go`, or from anywhere else under these names.
#
# Run with the editor closed:
#   UnrealEditor-Cmd.exe <path to StickToTheScript.uproject> -ExecutePythonScript=<path to this file> -unattended -nosplash -nullrhi -stdout

import glob
import os

import unreal

FOLDER = "/Game/Audio"
SOURCE = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()) + "Audio"

assets = unreal.EditorAssetLibrary
assets.make_directory(FOLDER)

brought = 0
for source in sorted(glob.glob(f"{SOURCE}/*.wav")):
    name = os.path.splitext(os.path.basename(source))[0]
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = FOLDER
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sound = unreal.load_asset(f"{FOLDER}/{name}")
    if not sound:
        unreal.log_warning(f"AUDIO {name}: not brought in")
        continue
    sound.set_editor_property("looping", name.startswith("M_"))
    assets.save_asset(f"{FOLDER}/{name}")
    brought += 1

unreal.log_warning(f"AUDIO brought in {brought} recordings from {SOURCE}")
unreal.SystemLibrary.quit_editor()
