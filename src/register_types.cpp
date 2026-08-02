#include "register_types.h"

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "al_source_node3d.h"
#include "openal/al_manager.h"
#include "transform_watcher.h"
#include "va_default_material.h"
#include "va_emitter.h"
#include "va_material.h"
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
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // ALSourceNode3D must be registered before VASource/VASourceRelative/
    // VASourceAmbient - it's their base class, and ClassDB::register_class
    // requires parent classes to already be registered.
    ClassDB::register_class<ALSourceNode3D>();

    ClassDB::register_class<va_godot::VAWorld>();
    ClassDB::register_class<va_godot::VAEmitter>();
    ClassDB::register_class<va_godot::VAMaterial>();
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
