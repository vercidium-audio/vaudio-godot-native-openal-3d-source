#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "al_source_node.h"
#include "al_source_node3d.h"
#include "al_source_node_relative.h"
#include "openal/al_manager.h"
#include "transform_watcher.h"
#include "va_conversion_plugin.h"
#include "va_default_material.h"
#include "va_emitter.h"
#include "va_listener.h"
#include "va_custom_material.h"
#include "va_openal_settings.h"
#include "va_primitive_ref.h"
#include "va_source.h"
#include "va_source_ambient.h"
#include "va_source_relative.h"
#include "va_world.h"

using namespace godot;

// Process-wide OpenAL device/context owner. Constructed/destroyed alongside
// the module rather than tied to any single node's lifetime, since there's
// only ever one OpenAL device for the whole plugin (see al_manager.h).
static ALManager al_manager;

void initialize_vaudio_godot_native_openal_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Scene tree right-click "Convert to VASource"/"Convert to
        // VASourceRelative" items - editor-only, so registered at the
        // EDITOR level rather than alongside the SCENE-level classes below.
        ClassDB::register_class<va_godot::ConversionContextMenuPlugin>();
        ClassDB::register_class<va_godot::VAConversionPlugin>();
        EditorPlugins::add_by_type<va_godot::VAConversionPlugin>();
        return;
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // ALSourceNode (and its ALSourceNode3D/ALSourceNodeRelative subclasses)
    // must be registered before VASource/VASourceRelative/VASourceAmbient -
    // they're their base classes, and ClassDB::register_class requires parent
    // classes to already be registered. ALSourceNode itself is never
    // instantiated directly (configure_source is pure virtual), so it's
    // registered as abstract.
    ClassDB::register_abstract_class<ALSourceNode>();
    ClassDB::register_class<ALSourceNode3D>();
    ClassDB::register_class<ALSourceNodeRelative>();

    ClassDB::register_class<va_godot::VAWorld>();
    ClassDB::register_class<va_godot::VAEmitter>();
    ClassDB::register_class<va_godot::VAListener>();
    ClassDB::register_class<va_godot::VACustomMaterial>();
    ClassDB::register_class<va_godot::VAOpenALSettings>();
    ClassDB::register_class<VADefaultMaterial>();
    ClassDB::register_class<VASource>();
    ClassDB::register_class<VASourceRelative>();
    ClassDB::register_class<VASourceAmbient>();

    // Internal helper classes - not part of the 6-node public surface, but
    // still need ClassDB registration since they're GDCLASS-derived and
    // instantiated via memnew (TransformWatcher as a real scene node,
    // VAPrimitiveRef as node metadata).
    ClassDB::register_class<TransformWatcher>();
    ClassDB::register_class<VAPrimitiveRef>();

    al_manager.initialize();
}

void uninitialize_vaudio_godot_native_openal_module(ModuleInitializationLevel p_level)
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

    al_manager.shutdown();
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT vaudio_godot_native_openal_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_vaudio_godot_native_openal_module);
	init_obj.register_terminator(uninitialize_vaudio_godot_native_openal_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
