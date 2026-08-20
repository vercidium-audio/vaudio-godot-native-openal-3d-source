#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include "al_source_node.h"
#include "al_source_node3d.h"
#include "al_source_node_relative.h"
#include "openal/al_manager.h"
#include "transform_watcher.h"
#include "va_conversion_plugin.h"
#include "va_debugger_plugin.h"
#include "va_default_material.h"
#include "va_device_name.h"
#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_input_stream_source.h"
#include "va_listener.h"
#include "va_custom_material.h"
#include "va_networked_stream_source.h"
#include "va_primitive_ref.h"
#include "va_raytraced_source_node3d.h"
#include "va_source.h"
#include "va_source_ambient.h"
#include "va_source_leech.h"
#include "va_source_relative.h"
#include "va_stream_source.h"
#include "va_visualisation.h"
#include "va_world.h"
#include "va_world_gizmo.h"
#include "va_world_lookup.h"

using namespace godot;

// Process-wide OpenAL device/context owner, registered as the "ALManager" Engine singleton so any
// script can call it directly. Heap-allocated with `memnew`
// inside initialize_vaudio_godot_native_openal_3d_module rather than a plain `static ALManager`,
// since ALManager is now a GDCLASS Object - see al_manager.h's class doc comment for why
// constructing one at CRT static-init time (before GDExtensionBinding exists) would crash.
static ALManager *al_manager = nullptr;

// Depth-first search for a descendant named scene_root_name - used instead of
// SceneTree::get_current_scene() below, which isn't reliable in a game that manually adds a scene
// as a plain child rather than via change_scene_to_*/change_scene_to_file (e.g. this plugin's own
// demo project's car_select.gd, which loads town_scene.tscn via get_parent().add_child(town) and
// never touches current_scene, leaving it permanently pointed at the CarSelect menu scene).
static Node *find_child_named_recursive(Node *node, const StringName &name)
{
    TypedArray<Node> children = node->get_children();

    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);

        if (child->get_name() == name)
            return child;

        if (Node *found = find_child_named_recursive(child, name))
            return found;
    }

    return nullptr;
}

// Receiving end of VADebuggerPlugin::sync_primitive - the editor process sends a
// "vaudio:sync_primitive" debugger message with the edited scene's root node name and a NodePath
// relative to it, whenever the "Vercidium Audio" Inspector material dropdown or permeation
// checkbox changes while the game is running. EditorInspectorPlugin controls can only edit the
// editor's own local copy of the scene, so this debugger-message capture is the only way those
// edits reach the running game's actual VAWorld - see va_debugger_plugin.h.
static bool on_debugger_message(const String &message, const Array &data)
{
    if (message != "sync_primitive" || data.size() < 4)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    if (!node)
    {
        VA_WARN(
            "Received a material/permeation edit from the editor for '", node_path,
            "', but no matching node exists under '", scene_root_name, "' (", scene_root->get_path(), ")");
        return true;
    }

    va_godot::VAWorld *va_world = va_godot::find_va_world(node);

    if (!va_world)
    {
        VA_WARN("Received a material/permeation edit from the editor for '", node->get_name(), "', but couldn't find a VAWorld in the running scene");
        return true;
    }

    // The running game has its own separate copy of this node - apply the metadata the editor's
    // local copy just had set/removed on it (see VAMaterialInspectorPlugin::sync_running_game)
    // before re-adding the primitive below, since add_primitive/get_material read it straight off
    // this node, not off anything sent directly.
    String material = data[2];
    StringName material_meta_key = va_godot::VAWorld::get_material_meta_key();

    if (material.is_empty())
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, material);

    Variant use_flat_transmission = data[3];
    StringName use_flat_transmission_meta_key = va_godot::VAWorld::get_use_flat_transmission_meta_key();

    if (use_flat_transmission.get_type() == Variant::NIL)
        node->remove_meta(use_flat_transmission_meta_key);
    else
        node->set_meta(use_flat_transmission_meta_key, use_flat_transmission);

    va_world->sync_primitive(node);

    return true;
}

// Rebuilds audio/vaudio/output_device's PROPERTY_HINT_ENUM hint_string from
// ALManager::get_available_devices(). Split out from register_project_settings() below so the
// "Refresh OpenAL Devices" tool menu item (va_conversion_plugin.cpp) can re-run just this part
// after soft_oal.dll has loaded, without touching the other three settings or their defaults.
// Godot's Project Settings dialog reads a property's hint_string when that row is drawn, not
// continuously - re-running this updates ProjectSettings' stored metadata immediately, but an
// already-open dialog only shows the new device list after its Output Device row is redrawn
// (e.g. switching away from and back to the General tab, or reopening the dialog).
//
// Strict enum, not PROPERTY_HINT_ENUM_SUGGESTION: for a Variant::STRING property, Godot's editor
// unconditionally injects an extra blank "" entry into an ENUM_SUGGESTION dropdown, with no way
// to suppress it - same issue VAOpenALSettings::_validate_property hit (see its doc comment in
// va_openal_settings.cpp), fixed the same way here. The device list being empty this early (before
// soft_oal.dll loads) isn't a problem for a strict enum either: DEFAULT_DEVICE_LABEL is always
// the one entry present, and ALManager::read_settings_from_project_settings() reads the raw
// setting value directly rather than validating it against hint_string, so an unrecognized saved
// device name is never lost - just not shown pre-selected until this is re-run.
void refresh_output_device_hint()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    PackedStringArray devices = ALManager::get_singleton() ? ALManager::get_singleton()->get_available_devices() : PackedStringArray();
    devices.insert(0, DEFAULT_DEVICE_LABEL);

    Dictionary output_device_info;
    output_device_info["name"] = "audio/vaudio/output_device";
    output_device_info["type"] = Variant::STRING;
    output_device_info["hint"] = PROPERTY_HINT_ENUM;
    output_device_info["hint_string"] = String(",").join(devices);
    settings->add_property_info(output_device_info);
}

// Registers this plugin's own audio/vaudio/* entries under Project Settings, matched to how
// Godot's own Project Settings > Audio > Driver > Device is read at startup before any scene
// loads - device_name/max_reverb_sends/sample_rate/ hrtf_enabled must exist here so ALManager::initialize()
// can read them before it opens the one-and-only device, below.
static void register_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // output_device: stored as DEFAULT_DEVICE_LABEL, not "", so the strict PROPERTY_HINT_ENUM
    // dropdown below always has a current value among its own entries - see DEFAULT_DEVICE_LABEL's
    // own doc comment in va_device_name.h. ALManager::read_settings_from_project_settings()
    // translates DEFAULT_DEVICE_LABEL back to "" ("driver default") when it reads this setting.
    if (!settings->has_setting("audio/vaudio/output_device"))
        settings->set_setting("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    settings->set_initial_value("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    refresh_output_device_hint();

    // max_reverb_sends: dev-only setting (not end-user-facing), default 1
    if (!settings->has_setting("audio/vaudio/max_reverb_sends"))
        settings->set_setting("audio/vaudio/max_reverb_sends", 1);

    settings->set_initial_value("audio/vaudio/max_reverb_sends", 1);

    Dictionary max_reverb_sends_info;
    max_reverb_sends_info["name"] = "audio/vaudio/max_reverb_sends";
    max_reverb_sends_info["type"] = Variant::INT;
    max_reverb_sends_info["hint"] = PROPERTY_HINT_RANGE;
    max_reverb_sends_info["hint_string"] = "0,16,or_greater";
    settings->add_property_info(max_reverb_sends_info);

    // max_mono_sources/max_stereo_sources: project-level settings set by the developer, matching
    // vaudio-godot-mono-openal-3d's ALManager.cs MaximumMonoSources/MaximumStereoSources -
    // defaults match that C# reference's own field initialisers (16/240).
    // Read once by ALManager::read_settings_from_project_settings() before the one-and-only device
    // open, same "not settable at runtime" shape as that C# reference (see MaximumMonoSources'
    // doc comment in ALManager.cs).
    if (!settings->has_setting("audio/vaudio/max_mono_sources"))
        settings->set_setting("audio/vaudio/max_mono_sources", 16);

    settings->set_initial_value("audio/vaudio/max_mono_sources", 16);

    Dictionary max_mono_sources_info;
    max_mono_sources_info["name"] = "audio/vaudio/max_mono_sources";
    max_mono_sources_info["type"] = Variant::INT;
    max_mono_sources_info["hint"] = PROPERTY_HINT_RANGE;
    max_mono_sources_info["hint_string"] = "0,256,or_greater";
    settings->add_property_info(max_mono_sources_info);

    if (!settings->has_setting("audio/vaudio/max_stereo_sources"))
        settings->set_setting("audio/vaudio/max_stereo_sources", 240);

    settings->set_initial_value("audio/vaudio/max_stereo_sources", 240);

    Dictionary max_stereo_sources_info;
    max_stereo_sources_info["name"] = "audio/vaudio/max_stereo_sources";
    max_stereo_sources_info["type"] = Variant::INT;
    max_stereo_sources_info["hint"] = PROPERTY_HINT_RANGE;
    max_stereo_sources_info["hint_string"] = "0,256,or_greater";
    settings->add_property_info(max_stereo_sources_info);

    // sample_rate: 0 means "driver default" - never shown to the user as 0.
    if (!settings->has_setting("audio/vaudio/sample_rate"))
        settings->set_setting("audio/vaudio/sample_rate", 0);

    settings->set_initial_value("audio/vaudio/sample_rate", 0);

    Dictionary sample_rate_info;
    sample_rate_info["name"] = "audio/vaudio/sample_rate";
    sample_rate_info["type"] = Variant::INT;
    sample_rate_info["hint"] = PROPERTY_HINT_ENUM;
    sample_rate_info["hint_string"] = "System Default:0,22050,44100,48000,96000";
    settings->add_property_info(sample_rate_info);

    // hrtf_enabled: default true
    if (!settings->has_setting("audio/vaudio/hrtf_enabled"))
        settings->set_setting("audio/vaudio/hrtf_enabled", true);

    settings->set_initial_value("audio/vaudio/hrtf_enabled", true);
}

void initialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Scene tree right-click "Convert to VASource"/"Convert to VASourceRelative" items - editor-only, so registered at the Editor level rather than alongside the Scene-level classes below.
        ClassDB::register_class<va_godot::ConversionContextMenuPlugin>();
        ClassDB::register_class<va_godot::VADeviceRefreshInspectorPlugin>();
        ClassDB::register_class<va_godot::VAMaterialInspectorPlugin>();
        ClassDB::register_class<va_godot::VAWorldGizmoPlugin>();
        ClassDB::register_class<va_godot::VADebuggerPlugin>();
        ClassDB::register_class<va_godot::VAConversionPlugin>();
        EditorPlugins::add_by_type<va_godot::VAConversionPlugin>();
        return;
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // AL* nodes must be registered before VA* nodes
    ClassDB::register_abstract_class<ALSourceNode>();
    ClassDB::register_class<ALSourceNode3D>();
    ClassDB::register_class<ALSourceNodeRelative>();

    ClassDB::register_class<va_godot::VAWorld>();
    ClassDB::register_class<va_godot::VAEmitter>();
    ClassDB::register_class<va_godot::VAListener>();
    ClassDB::register_class<va_godot::VACustomMaterial>();
    ClassDB::register_class<VADefaultMaterial>();
    ClassDB::register_abstract_class<VARaytracedSourceNode3D>();
    ClassDB::register_class<VASource>();
    ClassDB::register_class<VASourceRelative>();
    ClassDB::register_class<VASourceAmbient>();
    ClassDB::register_class<VASourceLeech>();
    ClassDB::register_class<VAStreamSource>();
    ClassDB::register_class<VAInputStreamSource>();
    ClassDB::register_class<VANetworkedStreamSource>();
    ClassDB::register_class<va_godot::VAVisualisation>();

    // Internal helper classes
    ClassDB::register_class<TransformWatcher>();
    ClassDB::register_class<VAPrimitiveRef>();

    register_project_settings();

    ClassDB::register_class<ALManager>();

    al_manager = memnew(ALManager);

    if (al_manager->initialize())
        Engine::get_singleton()->register_singleton("ALManager", al_manager);

    // Only the running game needs to receive VADebuggerPlugin's messages - EngineDebugger only
    // exists at all when running under the editor's debugger (see is_active()), and there's no
    // running game to sync in the editor process itself.
    if (!IS_EDITOR_HINT() && EngineDebugger::get_singleton() && EngineDebugger::get_singleton()->is_active())
        EngineDebugger::get_singleton()->register_message_capture("vaudio", callable_mp_static(&on_debugger_message));
}

void uninitialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        EditorPlugins::remove_by_type<va_godot::VAConversionPlugin>();
        return;
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    if (Engine::get_singleton()->has_singleton("ALManager"))
        Engine::get_singleton()->unregister_singleton("ALManager");

    al_manager->shutdown();
    memdelete(al_manager);
    al_manager = nullptr;

    if (EngineDebugger::get_singleton() && EngineDebugger::get_singleton()->has_capture("vaudio"))
        EngineDebugger::get_singleton()->unregister_message_capture("vaudio");
}

extern "C" {
GDExtensionBool GDE_EXPORT vaudio_godot_native_openal_3d_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_vaudio_godot_native_openal_3d_module);
	init_obj.register_terminator(uninitialize_vaudio_godot_native_openal_3d_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
