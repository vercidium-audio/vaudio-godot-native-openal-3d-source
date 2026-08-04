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
#include "va_primitive_ref.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace godot;

namespace va_godot
{

class VACustomMaterial;
class VAEmitter;

// Minimal VAWorld stub: proves the vaWorldCreate/vaWorldUpdate/vaWorldDestroy
// round-trip links and runs without crashing inside the Godot editor.
//
// Name collision note: the vaudio C SDK's opaque handle type is also called
// "VAWorld" (see vaudio.h), in the global namespace. This Godot node class
// lives in namespace va_godot instead of also being declared at global scope
// (a prefixed rename like GodotVAWorld was considered and rejected - see
// rename_plan.md - since GDCLASS's stringification would then show
// "GodotVAWorld" in the editor instead of "VAWorld"). Inside va_godot, the
// unqualified VAWorld always means this class; ::VAWorld (global namespace)
// always means the SDK's opaque handle type.
class VAWorld : public Node
{
    GDCLASS(VAWorld, Node);

private:
    ::VAWorld *world = nullptr;

    // VAWorldMaterials.cs's customMaterials dictionary port - keyed by
    // MaterialType id (>= 1000), populated by VACustomMaterial::_enter_tree via
    // register_custom_material.
    std::unordered_map<int, va_godot::VACustomMaterial *> custom_materials;

    // VAWorld.cs's listener field port - the one VAEmitter with
    // is_main_listener=true, set by register_emitter. Every other emitter is
    // automatically added as a target of this one (creation-order-driven,
    // matching VAWorld.cs's CreateEmitter wiring).
    va_godot::VAEmitter *listener = nullptr;

    // VAWorldVariables.cs's listenerReverbEffect port - the single global EFX
    // reverb slot, used by emitters/sources that don't affect grouped EAX
    // (or whose grouped_eax_index is invalid) - see get_reverb_effect.
    ALReverbEffect listener_reverb_effect;

    // VAWorldVariables.cs's groupedReverbEffects port - one EFX aux slot per
    // vaWorldGetGroupedEAX() entry, grown lazily in on_reverb_updated to
    // match vaWorldGetGroupedEAXCount(). Heap-allocated (not a plain
    // std::vector<ALReverbEffect>) since ALReverbEffect has no copy/move
    // constructor - its OpenAL handles can't survive a vector reallocation.
    std::vector<std::unique_ptr<ALReverbEffect>> grouped_reverb_effects;

    // VAWorldGodot.cs's OnReverbUpdated trampoline target - resolves back to
    // "this" via vaWorldGetUserData(world), set in the constructor via
    // vaWorldSetUserData. Reads vaEmitterGetEAX(listener's handle) and pushes
    // it into listener_reverb_effect.
    static void on_reverb_updated_trampoline(::VAWorld *world);
    void on_reverb_updated();

    // Emitters whose VAEmitter::on_emitter_removed already ran but whose
    // ::VAEmitter* handle hasn't been vaEmitterDestroy'd yet - see
    // VAEmitter::on_emitter_removed's doc comment for why destruction can't
    // happen synchronously inside the OnRemoved callback. Drained in
    // ~VAWorld, after vaWorldWait() has returned.
    std::vector<::VAEmitter *> pending_emitter_destroys;

    // VAWorldProperties.cs's PendingShutdown port - a plain passthrough to
    // vaWorldSetPendingShutdown. Purely an opt-in caller knob: setting this
    // ahead of an expected teardown lets vaWorldUpdate stop submitting new
    // raytracing work over the following frames, so the actual shutdown's
    // blocking vaWorldWait (still unconditional - see ~VAWorld) has less left
    // to drain. Matches the C# reference, which also always blocks in
    // Dispose() regardless of this flag.
    bool pending_shutdown = false;

    bool rendering_enabled = true;

    void init_scene();
    void on_node_added(Node *node);
    void on_node_removed(Node *node);

    // Editor-only counterpart to init_scene's tree walk - runs when
    // is_editor_hint() is true (so init_scene itself never runs), just to
    // surface get_material's unknown-material warning while editing. Doesn't
    // create any primitives/watchers - world is null in the editor.
    void validate_materials_in_editor(Node *node);

    // VAWorldMaterials.cs port: metadata-key based lookup, matching either a
    // registered custom VACustomMaterial node's MaterialName or a built-in name.
    VAMaterialType get_material(Node *node);

    // VAWorldPrimitives.cs port.
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

    // Called from VACustomMaterial::_enter_tree. Returns false (and logs a
    // conflicting-id error) if another VACustomMaterial already claimed this
    // MaterialType id - matches VAWorldMaterials.cs's duplicate check.
    bool register_custom_material(va_godot::VACustomMaterial *material);

    // Called from VAEmitter::_enter_tree (VAWorld.cs's CreateEmitter
    // listener-wiring port): if is_main_listener and no listener is set yet,
    // this becomes the listener; otherwise (once a listener exists) this
    // emitter is added as a target of the listener via vaEmitterAddTarget.
    // Also calls vaWorldAddEmitter. Logs a warning (doesn't reassign/throw)
    // if a second is_main_listener emitter shows up, or if a non-listener
    // emitter appears before any listener exists - matches VAWorld.cs.
    void register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener);

    // Exports all world settings, materials, primitives, and emitters to a
    // binary file via the SDK's vaWorldExport, for later use with
    // vaWorldImport. file_path is a native OS path (not a res:// URI) -
    // callers should resolve any Godot path via ProjectSettings::
    // globalize_path first. Returns false (and logs an error) on failure.
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

    // VAWorldReverb.cs's GetReverbEffect(vaudio.Emitter)/GetReverbEffect(VAEmitter)
    // port: emitters/sources that affect grouped EAX and have a valid
    // grouped-EAX index (i.e. they cast reverb rays - see
    // vaEmitterGetGroupedEAXIndex) resolve to their own grouped reverb slot;
    // everything else (including the listener itself) falls back to the
    // single global listener slot. Logs a warning and falls back to the
    // listener slot if the index is out of range for the current pool size,
    // matching the C#'s bounds check.
    ALReverbEffect *get_reverb_effect(::VAEmitter *emitter);

    // GroupedEAX stats - thin forwards to vaWorldGetGroupedEAXCount and
    // vaWorldGetGroupedEAX(world)[index]'s gainLF/gainHF/decayTime, read live
    // from the SDK (it already owns this array; no local caching needed).
    // index must be in [0, get_grouped_eax_count()).
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
        {
            VAResult result = vaWorldSetPendingShutdown(world, value);

            if (result != VA_SUCCESS)
            {
                // Should never trigger: the only failure mode is world being
                // NULL, already ruled out by the check above.
                UtilityFunctions::push_error(
                    "[vaudio-godot-native-openal] VAWorld::set_pending_shutdown failed (VAResult=", VAResultToString(result), ")");
            }
        }
    }

    bool get_rendering_enabled() const
    {
        return rendering_enabled;
    }

    void set_rendering_enabled(bool value)
    {
        rendering_enabled = value;

        if (world)
        {
            VAResult result = vaWorldSetRenderingEnabled(world, value);

            if (result != VA_SUCCESS)
            {
                // Should never trigger: the only failure mode is world being
                // NULL, already ruled out by the check above.
                UtilityFunctions::push_error(
                    "[vaudio-godot-native-openal] VAWorld::set_rendering_enabled failed (VAResult=", VAResultToString(result), ")");
            }
        }
    }

    // VAWorldProperties.cs port - remaining exported properties beyond
    // pending_shutdown. Each setter mirrors the C#'s pattern of clamping/
    // validating locally, caching the value, then forwarding to the native
    // handle if it already exists (it always does here - see the
    // constructor - but the null check is kept for symmetry with
    // set_pending_shutdown above).
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

    // Thin forwards to vaWorldGet{MainThread,Raytracing,Preparation,Analysis}Time
    // - average milliseconds per frame in each stage, read-only stats with no
    // local cache (always queried live from the SDK handle).
    double get_main_thread_time() const;
    double get_raytracing_time() const;
    double get_preparation_time() const;
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
