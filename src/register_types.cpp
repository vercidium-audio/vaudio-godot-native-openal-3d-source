#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "al_source.h"
#include "al_source3d.h"
#include "al_source_relative.h"
#include "openal/al_manager.h"
#include "transform_watcher.h"
#include "va_conversion_plugin.h"
#include "va_conversions.h"
#include "va_debugger_plugin.h"
#include "va_default_material.h"
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

// Receiving end of VADebuggerPlugin::sync_viewport_camera. Unlike sync_primitive/sync_material_properties this isn't addressed to a specific node - it's applied to the first VAWorld found in the running scene.
bool on_sync_viewport_camera(const Array &data)
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

    vaWorldSetCameraPosition(va_world->get_handle(), ToVAudio(position));
    vaWorldSetCameraYaw(va_world->get_handle(), rotation.y);
    vaWorldSetCameraPitch(va_world->get_handle(), rotation.x);
    vaWorldSetFieldOfView(va_world->get_handle(), Math::deg_to_rad(fov_degrees));

    return true;
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
        ClassDB::register_class<va_godot::VADebuggerBridge>();
        ClassDB::register_class<va_godot::VAConversionPlugin>();
        EditorPlugins::add_by_type<va_godot::VAConversionPlugin>();

        VA_LOG("Vercidium Audio (vaudio) plugin enabled");
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
