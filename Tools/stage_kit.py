# Shared helpers for the scripts that build stage levels (build_stage01.py, build_stage02.py, build_stage03.py).
# A stage script starts with begin(), places things, and ends with finish().

import unreal

MATERIALS = "/Game/Materials"
PROPS = "/Game/Props"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets = unreal.EditorAssetLibrary

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere")
BASE_MATERIAL = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")


def begin(level):
    """Opens the level empty: creates it, or loads it and removes everything in it."""
    if assets.does_asset_exist(level):
        level_editor.load_level(level)
        for existing in actors.get_all_level_actors():
            actors.destroy_actor(existing)
    else:
        level_editor.new_level(level)


def finish(level):
    level_editor.save_current_level()
    unreal.log(f"{level} built with {len(actors.get_all_level_actors())} actors.")
    unreal.SystemLibrary.quit_editor()


def cosmetic(target, name, value):
    # Looks only: a property that this engine version names differently must not stop the level being built.
    try:
        target.set_editor_property(name, value)
    except Exception as error:
        unreal.log_warning(f"Skipped {name}: {error}")


def _instance(name, parent):
    path = f"{MATERIALS}/{name}"
    instance = unreal.load_asset(path) if assets.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    # What it was told under another parent would stay in the file and mean nothing.
    unreal.MaterialEditingLibrary.clear_all_material_instance_parameters(instance)
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    return instance


def material(name, red, green, blue):
    """One flat colour."""
    instance = _instance(name, BASE_MATERIAL)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, "Color", unreal.LinearColor(red, green, blue, 1.0))
    assets.save_asset(f"{MATERIALS}/{name}")
    return instance


def surface(name, kind, tint=(1.0, 1.0, 1.0), tile=200.0, bump=1.0):
    """A real surface: one of the textures of Tools/build_materials.py (its SURFACES).
    One copy of it covers tile centimetres, however the block is stretched. The tint multiplies its colour."""
    library = unreal.MaterialEditingLibrary
    instance = _instance(name, unreal.load_asset(f"{MATERIALS}/M_Surface"))
    library.set_material_instance_texture_parameter_value(instance, "Colour", unreal.load_asset(f"{MATERIALS}/T_{kind}_C"))
    library.set_material_instance_texture_parameter_value(instance, "Normal", unreal.load_asset(f"{MATERIALS}/T_{kind}_N"))
    library.set_material_instance_vector_parameter_value(instance, "Tint", unreal.LinearColor(*tint, 1.0))
    library.set_material_instance_scalar_parameter_value(instance, "TileSize", tile)
    library.set_material_instance_scalar_parameter_value(instance, "Bump", bump)
    assets.save_asset(f"{MATERIALS}/{name}")
    return instance


def glow(name, red, green, blue, strength, parent="M_Light"):
    """Something that gives light of its own and is not shaded: a window, a bulb, a neon tube.
    The sets are exposed so that a strength near 30 is white on screen. Above that it blooms."""
    library = unreal.MaterialEditingLibrary
    instance = _instance(name, unreal.load_asset(f"{MATERIALS}/{parent}"))
    library.set_material_instance_vector_parameter_value(instance, "Color", unreal.LinearColor(red, green, blue, 1.0))
    library.set_material_instance_scalar_parameter_value(instance, "Strength", strength)
    assets.save_asset(f"{MATERIALS}/{name}")
    return instance


def block(label, mesh, look, location, scale, yaw=0.0, pitch=0.0):
    """A pitch of 90 lays a cylinder down along X."""
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(pitch=pitch, yaw=yaw, roll=0.0))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, look)
    return actor


def dress(label, name, location, yaw=0.0, scale=1.0, shadows=True):
    """A scanned set piece, one of SET_PIECES in Tools/build_scans.py, wearing its own materials. location is the point
    it stands on, or hangs from. A lamp with a light inside it must cast no shadow, or it shuts its own light in."""
    mesh = unreal.load_asset(f"{PROPS}/SM_Set{name}")
    if not mesh:
        raise RuntimeError(f"No set piece {name}. Run Tools/build_scans.py first.")
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.static_mesh_component.set_static_mesh(mesh)
    if not shadows:
        actor.static_mesh_component.set_cast_shadow(False)
    return actor


def sky(name, center, strength, turn=0.0, drop=0.0):
    """The night sky all round the set, as a photograph on the inside of a great ball. turn says which part of it is
    behind the set, in degrees, and drop lets its horizon down by that many. It lights nothing: the set has its own lamps."""
    library = unreal.MaterialEditingLibrary
    instance = _instance(name, unreal.load_asset(f"{MATERIALS}/M_Sky"))
    for parameter, value in (("Strength", strength), ("Turn", turn), ("Drop", drop)):
        library.set_material_instance_scalar_parameter_value(instance, parameter, value)
    assets.save_asset(f"{MATERIALS}/{name}")
    ball = block("Sky", SPHERE, instance, center, (140.0, 140.0, 140.0))
    ball.static_mesh_component.set_cast_shadow(False)
    # Nothing may touch the sky. A ball is solid to the physics, and the set is inside it: a knocked-out body, the one
    # thing on a set that collides, was pushed out through it and ended up on its skin, seventy metres away.
    ball.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    ball.static_mesh_component.set_collision_profile_name("NoCollision")
    ball.set_actor_enable_collision(False)
    cosmetic(ball.static_mesh_component, "affect_dynamic_indirect_lighting", False)
    cosmetic(ball.static_mesh_component, "affect_distance_field_lighting", False)
    return ball


def room(center, size, floor_look, wall_look, ceiling_look=None):
    """A closed box. center is the middle of the floor; size is width (X), depth (Y), height, all in centimetres."""
    x, y = center
    width, depth, height = size
    block("Floor", CUBE, floor_look, (x, y, -10), (width / 100, depth / 100, 0.2))
    block("Ceiling", CUBE, ceiling_look or wall_look, (x, y, height + 10), (width / 100, depth / 100, 0.2))
    block("Wall_Back", CUBE, wall_look, (x, y - depth / 2 - 10, height / 2), (width / 100, 0.2, height / 100))
    block("Wall_Front", CUBE, wall_look, (x, y + depth / 2 + 10, height / 2), (width / 100, 0.2, height / 100))
    block("Wall_Left", CUBE, wall_look, (x - width / 2 - 10, y, height / 2), (0.2, depth / 100, height / 100))
    block("Wall_Right", CUBE, wall_look, (x + width / 2 + 10, y, height / 2), (0.2, depth / 100, height / 100))


def crate(label, look, frame_look, location, scale, yaw=0.0):
    """A big wooden crate: a block with posts at the two corners the camera sees and rails round its top and bottom."""
    block(label, CUBE, look, location, scale, yaw)
    x, y, z = location
    width, depth, height = (side * 100.0 for side in scale)
    board = 13.0
    for side in (-1, 1):
        block(f"{label}_Post_{side}", CUBE, frame_look, (x + side * (width / 2 - board / 2 + 2), y + depth / 2 - board / 2 + 2, z),
              (board / 100, board / 100, height / 100), yaw)
        block(f"{label}_End_{side}", CUBE, frame_look, (x + side * (width / 2 + 1), y, z + height / 2 - board / 2), (0.03, depth / 100, board / 100), yaw)
    for name, lift in (("Top", height / 2 - board / 2), ("Bottom", -height / 2 + board / 2)):
        block(f"{label}_Rail_{name}", CUBE, frame_look, (x, y + depth / 2 + 1, z + lift), (width / 100, 0.03, board / 100), yaw)


def sign(label, text, location, size, color, strength, yaw=90.0):
    """Lettering, as paint (a strength near 15) or as neon (100 and up). With a yaw of 90 it faces the camera side of a set."""
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*location), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    actor.set_actor_label(label)
    letters = actor.text_render
    letters.set_text(unreal.Text(text))
    letters.set_world_size(size)
    letters.set_text_material(glow(f"MI_{label}", color[0], color[1], color[2], strength, "M_Sign"))
    letters.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    letters.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    return actor


def aim(location, target):
    return unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*location), unreal.Vector(*target))


def tagged(actor_class, name, location, rotation=None):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), rotation or unreal.Rotator())
    actor.set_actor_label(name)
    actor.tags = [unreal.Name(name)]
    return actor


def marks(places, tape_look):
    """Where the performers stand. The game finds these by tag. Each gets a cross of tape on the floor, as on a real set."""
    for name, location in places.items():
        tagged(unreal.TargetPoint, name, location)
        block(f"Tape_{name}_Across", CUBE, tape_look, (location[0], location[1], 0.3), (0.3, 0.04, 0.005))
        block(f"Tape_{name}_Along", CUBE, tape_look, (location[0], location[1], 0.3), (0.04, 0.2, 0.005))


def film_cameras(places):
    """The three film cameras, by tag: (location, look-at point, field of view). The final cut re-aims the close and low ones."""
    camera_class = unreal.load_class(None, "/Script/StickToTheScript.FightCamera")
    for name, (location, target, field_of_view) in places.items():
        camera = tagged(camera_class, name, location, aim(location, target))
        camera.set_editor_property("base_field_of_view", field_of_view)


def spot_light(label, location, target, candelas, color, cone_degrees, shadows=True):
    light = actors.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(*location), aim(location, target))
    light.set_actor_label(label)
    component = light.light_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    component.set_intensity(candelas)
    component.set_light_color(unreal.LinearColor(*color, 1.0))
    component.set_outer_cone_angle(cone_degrees)
    component.set_inner_cone_angle(cone_degrees * 0.6)
    component.set_attenuation_radius(1600.0)
    cosmetic(component, "volumetric_scattering_intensity", 1.5)
    if not shadows:
        component.set_cast_shadows(False)
    return light


def fill_light(location, candelas, size, color=(1.0, 1.0, 1.0)):
    """A soft panel pointing straight down."""
    fill = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(*location), unreal.Rotator(pitch=-90.0, yaw=0.0, roll=0.0))
    fill.set_actor_label("Light_Fill")
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    fill.light_component.set_intensity(candelas)
    fill.light_component.set_light_color(unreal.LinearColor(*color, 1.0))
    fill.light_component.set_editor_property("source_width", size)
    fill.light_component.set_editor_property("source_height", size)
    fill.light_component.set_attenuation_radius(1500.0)


def haze(density, clear_beyond=0.0):
    """A little fog, so the spots show as beams. Under a sky it stops at clear_beyond centimetres, or it would hide it."""
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("Haze")
    cosmetic(fog.component, "fog_density", density)
    cosmetic(fog.component, "enable_volumetric_fog", True)
    if clear_beyond:
        cosmetic(fog.component, "fog_cutoff_distance", clear_beyond)


def grade(exposure, vignette):
    """A fixed exposure, so a set stays dark with pools of light instead of being brightened to an even grey."""
    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("Grade")
    cosmetic(volume, "unbound", True)
    look = volume.get_editor_property("settings")
    for name, value in {
        "override_auto_exposure_min_brightness": True,
        "auto_exposure_min_brightness": exposure,
        "override_auto_exposure_max_brightness": True,
        "auto_exposure_max_brightness": exposure,
        "override_vignette_intensity": True,
        "vignette_intensity": vignette,
        # A film look: lights that glow, grain, a little colour fringe at the edges, and stronger contrast and colour.
        "override_bloom_intensity": True,
        "bloom_intensity": 0.9,
        "override_film_grain_intensity": True,
        "film_grain_intensity": 0.3,
        "override_scene_fringe_intensity": True,
        "scene_fringe_intensity": 0.5,
        "override_color_contrast": True,
        "color_contrast": unreal.Vector4(1.12, 1.12, 1.12, 1.0),
        "override_color_saturation": True,
        "color_saturation": unreal.Vector4(1.12, 1.12, 1.12, 1.0),
    }.items():
        cosmetic(look, name, value)
    volume.set_editor_property("settings", look)
