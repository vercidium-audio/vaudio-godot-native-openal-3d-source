#pragma once

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/csg_box3d.hpp>
#include <godot_cpp/classes/csg_cylinder3d.hpp>
#include <godot_cpp/classes/csg_sphere3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3.hpp>

extern "C"
{
#include <vaudio.h>
}

#include "openal/al_reverb.h"
#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_primitive_ref.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace godot;

namespace va_godot
{

class VACustomMaterial;
class VAEmitter;


// Name collision: the vaudio C SDK's opaque handle type is also called "VAWorld" (global namespace); inside va_godot, 'VAWorld' means this class and '::VAWorld' the SDK handle.
// This is a Node3D purely so the editor can draw a gizmo for the bounds_position/bounds_size AABB (see VAWorldGizmoPlugin) - the node's own transform is otherwise unused by vaudio.
class VAWorld : public Node3D
{
    GDCLASS(VAWorld, Node3D);

private:
    ::VAWorld *world = nullptr;

    std::unordered_map<int, va_godot::VACustomMaterial *> custom_materials;

    va_godot::VAEmitter *listener = nullptr;

    // Emitters that invoked register_emitter() before the listener existed; drained when the main listener registers.
    std::vector<va_godot::VAEmitter *> pending_targets;

    ALReverbEffect listener_reverb_effect;
    std::vector<std::unique_ptr<ALReverbEffect>> grouped_reverb_effects;

    static void on_reverb_updated_trampoline(::VAWorld *world);
    void on_reverb_updated();

    bool warned_missing_listener = false;

    // Set at the start of _exit_tree - suppresses the missing-listener warning for the in-flight reverb-updated
    // callback that can still fire once during teardown, after the listener unregisters but before this node is destroyed.
    bool is_shutting_down = false;

    // Emitters whose on_emitter_removed already ran but whose ::VAEmitter* handle hasn't been vaEmitterDestroy'd yet
    // (see on_emitter_removed's comment for why destruction can't happen synchronously); drained in ~VAWorld after vaWorldWait().
    std::vector<::VAEmitter *> pending_emitter_destroys;

    bool pending_shutdown = false;
    bool rendering_enabled = true;

    void init_scene();
    void on_node_added(Node *node);
    void on_node_removed(Node *node);

    // Reports unknown material metadata strings when in the editor, not used at runtime.
    void validate_materials_in_editor(Node *node);

    VAMaterialType get_material(Node *node);

    void add_primitive(Node *node, VAMaterialType material, bool use_flat_transmission, bool recursive);
    void remove_primitive(Node *node, bool recursive);

    VAPrimitiveRef *attach_watcher(Node3D *node, void *primitive, VAPrimitiveKind kind, std::function<void()> update);

    void create_primitive(CSGBox3D *csg_box, VAMaterialType material);
    void create_primitive(CSGCylinder3D *csg_cylinder, VAMaterialType material);
    void create_primitive(CSGSphere3D *csg_sphere, VAMaterialType material);
    void create_primitive(CollisionShape3D *collision_shape, VAMaterialType material);
    void create_primitive(MeshInstance3D *mesh_instance, VAMaterialType material, bool use_flat_transmission);

    void update_collision_shape_primitive(CollisionShape3D *collision_shape, VAPrimitiveRef *ref);

protected:
    static void _bind_methods();

public:
    VAWorld();
    ~VAWorld();

    void _ready() override;
    void _exit_tree() override;
    void _process(double delta) override;
    void _validate_property(PropertyInfo &p_property) const;

    ::VAWorld *get_handle() const
    {
        return world;
    }

    // Returns false if another VACustomMaterial already claimed the same ID.
    bool register_custom_material(va_godot::VACustomMaterial *material);

    // Re-registers a single node's raytracing primitive after its "Vercidium Audio" material/permeation metadata changed
    // in the Inspector while the game is running. Only called on the running game's own VAWorld via the debugger-message
    // capture in register_types.cpp - a custom EditorInspectorPlugin control has no way to reach this directly.
    void sync_primitive(Node *node);

    // The 23 built-in material names, for editor tooling such as VAMaterialInspectorPlugin's material dropdown.
    static PackedStringArray get_builtin_material_names();

    // Node metadata key storing a node's chosen material name; single source of truth shared with MaterialMetaKey() in va_world_primitives.cpp.
    static String get_material_meta_key();

    // Node metadata key storing a node's "Use Flat Transmission" override; shared with UseFlatTransmissionMetaKey() in va_world_primitives.cpp.
    static String get_use_flat_transmission_meta_key();

    void register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener);

    // Removes this emitter from pending_targets if it's still waiting there for a listener to appear.
    void unregister_pending_target(va_godot::VAEmitter *emitter);

    // Called from VAEmitter::_exit_tree when the current listener leaves the tree, so this world doesn't keep a dangling pointer.
    void unregister_listener(va_godot::VAEmitter *emitter);

    // Exports all world settings, materials, primitives, and emitters to a binary file. file_path is a native OS path
    // (not res://) - callers should resolve via ProjectSettings::globalize_path first. Returns false on failure.
    bool export_to_file(const String &file_path);

    va_godot::VAEmitter *get_listener() const
    {
        return listener;
    }

    // Called from VAEmitter::on_emitter_removed. Queues the now-detached handle for vaEmitterDestroy, deferred until
    // ~VAWorld runs it after vaWorldWait() returns - destroying an emitter synchronously inside its own OnRemoved
    // callback can race the world's in-flight raytracing drain. TODO: have the SDK guard against this itself.
    void defer_emitter_destroy(::VAEmitter *emitter)
    {
        pending_emitter_destroys.push_back(emitter);
    }

    ALReverbEffect *get_reverb_effect(::VAEmitter *emitter);

    int get_grouped_eax_count() const;
    float get_grouped_eax_gain_lf(int index) const;
    float get_grouped_eax_gain_hf(int index) const;
    float get_grouped_eax_decay_time(int index) const;

    bool get_pending_shutdown() const
    {
        return pending_shutdown;
    }

    void set_pending_shutdown(bool value)
    {
        pending_shutdown = value;

        if (world)
            vaWorldSetPendingShutdown(world, value);
    }

    bool get_rendering_enabled() const
    {
        return rendering_enabled;
    }

    void set_rendering_enabled(bool value)
    {
        rendering_enabled = value;

        // The debug render window creates its own native OpenGL (WGL) context; when the project's renderer is also
        // gl_compatibility (real WGL, not ANGLE), both run on the main thread and corrupt each other's GL state,
        // observed to crash the engine deterministically within seconds - refuse until the root cause is fixed upstream.
        if (value && RenderingServer::get_singleton()->get_current_rendering_method() == "gl_compatibility")
        {
            UtilityFunctions::push_warning("VAWorld: rendering_enabled was not applied because the "
                "project's renderer is gl_compatibility - the vaudio debug render window uses its own "
                "native OpenGL context, which is known to crash the engine when the project also uses "
                "gl_compatibility. Switch to Forward+ or Mobile to use the debug render window.");
            rendering_enabled = false;
            return;
        }

        if (world)
            vaWorldSetRenderingEnabled(world, value);
    }

    // Shadows the inherited Node3D::position property so moving this node also updates vaWorldSetPosition - see va_world_properties.cpp.
    Vector3 get_position() const
    {
        return Node3D::get_position();
    }
    void set_position(const Vector3 &value);

    Vector3 get_bounds_size() const
    {
        return bounds_size;
    }
    void set_bounds_size(Vector3 value);

    Color get_bounds_color() const
    {
        return bounds_color;
    }
    void set_bounds_color(Color value);

    float get_epsilon() const
    {
        return epsilon;
    }
    void set_epsilon(float value);

    bool get_world_is_indoors() const
    {
        return world_is_indoors;
    }
    void set_world_is_indoors(bool value);

    int get_maximum_grouped_eax_count() const
    {
        return maximum_grouped_eax_count;
    }
    void set_maximum_grouped_eax_count(int value);

    float get_meters_per_unit() const
    {
        return meters_per_unit;
    }
    void set_meters_per_unit(float value);

    float get_speed_of_sound() const
    {
        return speed_of_sound;
    }
    void set_speed_of_sound(float value);

    float get_master_volume() const
    {
        return master_volume;
    }
    void set_master_volume(float value);

    int get_distance_model() const
    {
        return distance_model;
    }
    void set_distance_model(int value);

    bool get_reverb_only() const
    {
        return reverb_only;
    }
    void set_reverb_only(bool value);

    float get_humidity() const
    {
        return humidity;
    }
    void set_humidity(float value);

    float get_temperature() const
    {
        return temperature;
    }
    void set_temperature(float value);

    float get_pressure() const
    {
        return pressure;
    }
    void set_pressure(float value);

    float get_reference_frequency_lf() const
    {
        return reference_frequency_lf;
    }
    void set_reference_frequency_lf(float value);

    float get_reference_frequency_hf() const
    {
        return reference_frequency_hf;
    }
    void set_reference_frequency_hf(float value);

    bool get_emitters_outside_the_world_are_muffled() const
    {
        return emitters_outside_the_world_are_muffled;
    }
    void set_emitters_outside_the_world_are_muffled(bool value);

    int get_maximum_concurrency_level() const
    {
        return maximum_concurrency_level;
    }
    void set_maximum_concurrency_level(int value);

    int get_work_item_count() const
    {
        return work_item_count;
    }
    void set_work_item_count(int value);

    double get_main_thread_time() const;
    double get_preparation_time() const;
    double get_raytracing_time() const;
    double get_analysis_time() const;

private:
    Vector3 bounds_size = Vector3(200, 100, 200);
    Color bounds_color = Color(0.0f, 0.0f, 0.0f, 0.25f);
    float epsilon = 0.01f;
    bool world_is_indoors = false;
    int maximum_grouped_eax_count = 3;
    float meters_per_unit = 1.0f;
    float speed_of_sound = 343.0f;
    float master_volume = 1.0f;
    int distance_model = AL_INVERSE_DISTANCE_CLAMPED;
    bool reverb_only = false;
    float humidity = 0.1f;
    float temperature = 26.0f;
    float pressure = 101325.0f;
    float reference_frequency_lf = 300.0f;
    float reference_frequency_hf = 4000.0f;
    bool emitters_outside_the_world_are_muffled = true;
    int maximum_concurrency_level = 0;
    int work_item_count = 128;
};

} // namespace va_godot
