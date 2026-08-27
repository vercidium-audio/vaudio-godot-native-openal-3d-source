#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include "al_source.h"
#include "al_source3d.h"
#include "al_source_relative.h"
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
#include "va_material_properties_inspector_plugin.h"
#include "va_networked_stream_source.h"
#include "va_node_gizmo.h"
#include "va_primitive_ref.h"
#include "va_raytraced_source.h"
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

// Process-wide OpenAL device/context owner, registered as the "ALManager" Engine singleton. Heap-allocated with
// `memnew` rather than a plain `static ALManager`, since constructing a GDCLASS Object at CRT static-init time (before GDExtensionBinding exists) would crash.
static ALManager *al_manager = nullptr;

// Depth-first search for a descendant named scene_root_name - used instead of SceneTree::get_current_scene(), which isn't
// reliable in a game that manually adds a scene as a plain child rather than via change_scene_to_*/change_scene_to_file.
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

// VADefaultMaterial/VACustomMaterial nodes are always direct children of a VAWorld node (see both classes' doc
// comments). existing_node is either nullptr (nothing at node_path at all) or a plain Node the editor's own Live
// Edit already created there (see on_sync_material_properties) - either way it's replaced with a freshly
// constructed, correctly typed node under the same parent, so the new node's own _enter_tree runs and registers it
// with the running ::VAWorld. Returns nullptr (with no node created) if the parent isn't a VAWorld already present
// in the running scene.
static Node *replace_with_material_node(Node *scene_root, const NodePath &node_path, Node *existing_node, const String &node_name,
    bool is_custom_material, int material_type, const String &custom_material_name)
{
    Node *parent;

    if (existing_node)
    {
        parent = existing_node->get_parent();

        if (parent)
            parent->remove_child(existing_node);

        existing_node->queue_free();
    }
    else
    {
        int64_t name_count = node_path.get_name_count();
        if (name_count < 2)
            return nullptr;

        parent = scene_root->get_node_or_null(node_path.slice(0, name_count - 1));
    }

    if (!Object::cast_to<va_godot::VAWorld>(parent))
        return nullptr;

    Node *node;

    if (is_custom_material)
    {
        va_godot::VACustomMaterial *custom_material = memnew(va_godot::VACustomMaterial);
        custom_material->set_material_name(custom_material_name);
        node = custom_material;
    }
    else
    {
        VADefaultMaterial *default_material = memnew(VADefaultMaterial);
        default_material->set_material_type(material_type);
        node = default_material;
    }

    node->set_name(node_name);
    parent->add_child(node);

    return node;
}

// Relays a VADefaultMaterial/VACustomMaterial property edit made in the Inspector while the game is running - see
// VAMaterialPropertiesInspectorPlugin. Unlike on_debugger_message's "sync_primitive" handling below, this never
// touches node metadata: the target node's own apply_properties_from_editor pushes the new values straight into
// the ::VAWorld material already tracking it.
//
// If the node doesn't exist yet, or exists but is a plain Node rather than a VADefaultMaterial/VACustomMaterial,
// it's (re)created here so its own _enter_tree can register it with the running ::VAWorld the normal way. The
// "exists but plain Node" case isn't a custom-protocol gap like sync_primitive's - the editor's own Live Edit
// feature already creates a matching node in the running game whenever one is added in the editor's local scene
// copy, but Live Edit only replicates the node's built-in Godot class, not any script/GDExtension class attached
// to it, so the node Live Edit creates is always a plain Node.
static bool on_sync_material_properties(const Array &data)
{
    // scene_root_name, node_path, node_name, is_custom_material, material_type, custom_material_name,
    // 7 material floats, color
    if (data.size() < 14)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material property edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material property edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    String node_name = data[2];
    bool is_custom_material = data[3];
    int material_type = data[4];
    String custom_material_name = data[5];

    float absorption_lf = data[6];
    float absorption_hf = data[7];
    float scattering = data[8];
    float transmission_lf = data[9];
    float transmission_hf = data[10];
    float flat_transmission_lf = data[11];
    float flat_transmission_hf = data[12];
    Color color = data[13];

    if (!Object::cast_to<VADefaultMaterial>(node) && !Object::cast_to<va_godot::VACustomMaterial>(node))
        node = replace_with_material_node(scene_root, node_path, node, node_name, is_custom_material, material_type, custom_material_name);

    if (VADefaultMaterial *default_material = Object::cast_to<VADefaultMaterial>(node))
        default_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else if (va_godot::VACustomMaterial *custom_material = Object::cast_to<va_godot::VACustomMaterial>(node))
        custom_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else
        VA_WARN(
            "Received a material property edit from the editor for '", node_path,
            "', but no matching VADefaultMaterial/VACustomMaterial node exists under '", scene_root_name, "' (", scene_root->get_path(),
            "), and its parent VAWorld node doesn't exist in the running scene either - restart the running game to pick it up");

    return true;
}

// Receiving end of VADebuggerPlugin::sync_viewport_camera. Unlike sync_primitive/sync_material_properties this isn't addressed to a specific node - it's applied to the first VAWorld found in the running scene.
static bool on_sync_viewport_camera(const Array &data)
{
    if (data.size() < 3)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
        return true;

    va_godot::VAWorld *va_world = va_godot::find_va_world_recursive(tree_root);

    if (!va_world || !va_world->get_sync_viewport() || !va_world->get_handle() || !va_world->get_rendering_enabled())
        return true;

    Vector3 position = data[0];
    Vector3 rotation = data[1];
    float fov_degrees = data[2];

    vaWorldSetManualCamera(va_world->get_handle(), false);
    vaWorldSetCameraPosition(va_world->get_handle(), ToVAudio(position));
    vaWorldSetCameraYaw(va_world->get_handle(), rotation.y);
    vaWorldSetCameraPitch(va_world->get_handle(), rotation.x);
    vaWorldSetFieldOfView(va_world->get_handle(), Math::deg_to_rad(fov_degrees));

    return true;
}

// Receiving end of VADebuggerPlugin::sync_primitive - the editor sends a "vaudio:sync_primitive" debugger message
// with the edited scene's root node name and a NodePath relative to it, whenever the "Vercidium Audio" Inspector
// material dropdown or permeation checkbox changes while the game is running. EditorInspectorPlugin controls can only
// edit the editor's own local copy of the scene, so this debugger-message capture is the only way those edits reach the running game's actual VAWorld.
static bool on_debugger_message(const String &message, const Array &data)
{
    if (message == "sync_material_properties")
        return on_sync_material_properties(data);

    if (message == "sync_viewport_camera")
        return on_sync_viewport_camera(data);

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

    // The running game has its own separate copy of this node - apply the metadata the editor's local copy just had
    // set/removed on it before re-adding the primitive below, since add_primitive/get_material read it straight off this node.
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

// Rebuilds audio/vaudio/output_device's PROPERTY_HINT_ENUM hint_string from ALManager::get_available_devices(). Split
// out from register_project_settings() so the "Refresh OpenAL Devices" tool menu item can re-run just this part after
// soft_oal.dll has loaded; an already-open Project Settings dialog only shows the new list after its row is redrawn.
// Strict enum, not PROPERTY_HINT_ENUM_SUGGESTION: Godot's editor unconditionally injects a blank "" entry into an
// ENUM_SUGGESTION dropdown for a Variant::STRING property, with no way to suppress it.
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

// Registers this plugin's own audio/vaudio/* entries under Project Settings - device_name/max_reverb_sends/sample_rate/
// hrtf_enabled must exist here so ALManager::initialize() can read them before it opens the one-and-only device, below.
static void register_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // output_device: stored as DEFAULT_DEVICE_LABEL, not "", so the strict PROPERTY_HINT_ENUM dropdown below always has
    // a current value among its own entries. ALManager::read_settings_from_project_settings() translates it back to "" ("driver default").
    if (!settings->has_setting("audio/vaudio/output_device"))
        settings->set_setting("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    settings->set_initial_value("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    refresh_output_device_hint();

    // max_reverb_sends: dev-only setting (not end-user-facing), default 1. Floor of 1, not 0 - a
    // value of 0 requests zero auxiliary sends from the driver, which makes every
    // AL_AUXILIARY_SEND_FILTER call on any source fail with "Invalid send 0"
    // (al_manager.cpp's read_settings_from_project_settings() clamps this the same way at read
    // time, in case an existing project.godot already has an explicit 0 saved from before this
    // floor existed).
    if (!settings->has_setting("audio/vaudio/max_reverb_sends"))
        settings->set_setting("audio/vaudio/max_reverb_sends", 1);

    settings->set_initial_value("audio/vaudio/max_reverb_sends", 1);

    Dictionary max_reverb_sends_info;
    max_reverb_sends_info["name"] = "audio/vaudio/max_reverb_sends";
    max_reverb_sends_info["type"] = Variant::INT;
    max_reverb_sends_info["hint"] = PROPERTY_HINT_RANGE;
    max_reverb_sends_info["hint_string"] = "1,16,or_greater";
    settings->add_property_info(max_reverb_sends_info);

    // max_mono_sources/max_stereo_sources: project-level settings set by the developer, matching ALManager.cs's
    // MaximumMonoSources/MaximumStereoSources defaults (16/240). Read once before the one-and-only device open, not settable at runtime.
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
        // Editor-only classes, registered at the Editor level rather than alongside the Scene-level classes below.
        ClassDB::register_class<va_godot::ConversionContextMenuPlugin>();
        ClassDB::register_class<va_godot::VADeviceRefreshInspectorPlugin>();
        ClassDB::register_class<va_godot::VAMaterialInspectorPlugin>();
        ClassDB::register_class<va_godot::VAMaterialPropertiesInspectorPlugin>();
        ClassDB::register_class<va_godot::VAWorldGizmoPlugin>();
        ClassDB::register_class<va_godot::VANodeGizmoPlugin>();
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
    ClassDB::register_abstract_class<ALSource>();
    ClassDB::register_class<ALSource3D>();
    ClassDB::register_class<ALSourceRelative>();

    ClassDB::register_class<va_godot::VAWorld>();
    ClassDB::register_class<va_godot::VAEmitter>();
    ClassDB::register_class<va_godot::VAListener>();
    ClassDB::register_class<va_godot::VACustomMaterial>();
    ClassDB::register_class<VADefaultMaterial>();
    ClassDB::register_abstract_class<VARaytracedSource>();
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

    // Only the running game needs to receive VADebuggerPlugin's messages - there's no running game to sync in the editor process itself.
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
