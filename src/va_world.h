#pragma once

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/csg_box3d.hpp>
#include <godot_cpp/classes/csg_cylinder3d.hpp>
#include <godot_cpp/classes/csg_sphere3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
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


// Name collision: the vaudio C SDK's opaque handle type is also called "VAWorld" (see vaudio.h), in the global namespace.
// Inside the va_godot namespace, 'VAWorld' always means this class, and '::VAWorld' means the SDK's opaque handle type.
class VAWorld : public Node
{
    GDCLASS(VAWorld, Node);

private:
    ::VAWorld *world = nullptr;

    // Dictionary of registered custom materials
    std::unordered_map<int, va_godot::VACustomMaterial *> custom_materials;

    // The single main listener for this world
    va_godot::VAEmitter *listener = nullptr;

    // Contains emitters that invoked register_emitter() before the listener existed.
    // Drained when the main listener invokes register_emitter().
    std::vector<va_godot::VAEmitter *> pending_targets;

    // Single AL reverb effect for the listener
    ALReverbEffect listener_reverb_effect;

    // List of Grouped EAX reverb effects
    std::vector<std::unique_ptr<ALReverbEffect>> grouped_reverb_effects;

    // Callback from the vaWorld when reverb is updated
    static void on_reverb_updated_trampoline(::VAWorld *world);
    void on_reverb_updated();

    // Warn the user once if we don't have a listener
    bool warned_missing_listener = false;

    // Emitters whose VAEmitter::on_emitter_removed already ran but whose ::VAEmitter* handle hasn't been vaEmitterDestroy'd yet
    //  See VAEmitter::on_emitter_removed's doc comment for why destruction can't happen synchronously inside the OnRemoved callback.
    //  Drained in ~VAWorld, after vaWorldWait() has returned.
    std::vector<::VAEmitter *> pending_emitter_destroys;

    bool pending_shutdown = false;
    bool rendering_enabled = true;

    void init_scene();
    void on_node_added(Node *node);
    void on_node_removed(Node *node);

    // Reports unknown material metadata strings when in the editor, not used at runtime
    void validate_materials_in_editor(Node *node);

    // Get the type of a custom or default material
    VAMaterialType get_material(Node *node);

    void add_primitive(Node *node, VAMaterialType material, bool recursive);
    void remove_primitive(Node *node, bool recursive);

    VAPrimitiveRef *attach_watcher(Node3D *node, void *primitive, VAPrimitiveKind kind, std::function<void()> update);

    void create_primitive(CSGBox3D *csg_box, VAMaterialType material);
    void create_primitive(CSGCylinder3D *csg_cylinder, VAMaterialType material);
    void create_primitive(CSGSphere3D *csg_sphere, VAMaterialType material);
    void create_primitive(CollisionShape3D *collision_shape, VAMaterialType material);
    void create_primitive(MeshInstance3D *mesh_instance, VAMaterialType material);

    void update_collision_shape_primitive(CollisionShape3D *collision_shape, VAPrimitiveRef *ref);

protected:
    static void _bind_methods();

public:
    VAWorld();
    ~VAWorld();

    void _ready() override;
    void _exit_tree() override;
    void _process(double delta) override;

    ::VAWorld *get_handle() const
    {
        return world;
    }

    // Returns false if another VACustomMaterial already claimed the same ID
    bool register_custom_material(va_godot::VACustomMaterial *material);

    void register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener);

    // Removes this emitter from pending_targets if it's still waiting there for a listener to appear
    void unregister_pending_target(va_godot::VAEmitter *emitter);

    // Export all world settings, materials, primitives, and emitters to a binary file
    // file_path is a native OS path (not a res:// URI) - callers should resolve any Godot path via ProjectSettings::globalize_path first.
    // Returns false (and logs an error) on failure.
    bool export_to_file(const String &file_path);

    va_godot::VAEmitter *get_listener() const
    {
        return listener;
    }

    // Called from VAEmitter::on_emitter_removed. Queues the now-detached
    // handle for vaEmitterDestroy, deferred until ~VAWorld runs it after
    // vaWorldWait() returns - see on_emitter_removed's doc comment for the
    // use-after-free this avoids (destroying an emitter synchronously inside
    // its own OnRemoved callback can race the world's in-flight raytracing
    // drain). TODO: revisit once the SDK itself can guard against this -
    // e.g. vaEmitterDestroy returning an error code if called while the
    // world that owned this emitter is mid-vaWorldWait/from within an
    // OnRemoved callback - tracked as a follow-up, not fixed here.
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

        if (world)
            vaWorldSetRenderingEnabled(world, value);
    }

    Vector3 get_position() const
    {
        return position;
    }
    void set_position(Vector3 value);

    Vector3 get_size() const
    {
        return size;
    }
    void set_size(Vector3 value);

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
    Vector3 position = Vector3(-100, 0, -100);
    Vector3 size = Vector3(200, 100, 200);
    float epsilon = 0.01f;
    bool world_is_indoors = false;
    int maximum_grouped_eax_count = 3;
    float meters_per_unit = 1.0f;
    float speed_of_sound = 343.0f;
    float humidity = 0.1f;
    float temperature = 26.0f;
    float pressure = 101325.0f;
    float reference_frequency_lf = 300.0f;
    float reference_frequency_hf = 4000.0f;
    bool emitters_outside_the_world_are_muffled = true;
    int maximum_concurrency_level = 4; // overwritten in VAWorld::VAWorld() with processor count - 1
    int work_item_count = 128;
};

} // namespace va_godot
