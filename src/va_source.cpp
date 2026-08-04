#include "va_source.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VASource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_play_when_raytracing_completes"), &VASource::get_play_when_raytracing_completes);
    ClassDB::bind_method(D_METHOD("set_play_when_raytracing_completes", "value"), &VASource::set_play_when_raytracing_completes);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "play_when_raytracing_completes"), "set_play_when_raytracing_completes", "get_play_when_raytracing_completes");

    // Read-only muffling stats - no ADD_PROPERTY, same rationale as
    // VAWorld's timing stats (called directly from GDScript like methods).
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VASource::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VASource::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VASource::is_raytraced);

    // Direct port of vaudio-godot-openal's VASourceProperties.cs groups
    // (Reverb/Muffling/Ambience/Visualisation/Advanced) - see va_source.h for
    // why this is a subset of VAEmitter's own property surface. Debug
    // Rendering colors are not ported, same rationale as VAEmitter.
    ADD_GROUP("Reverb", "");

    ClassDB::bind_method(D_METHOD("get_reverb_ray_count"), &VASource::get_reverb_ray_count);
    ClassDB::bind_method(D_METHOD("set_reverb_ray_count", "value"), &VASource::set_reverb_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_ray_count"), "set_reverb_ray_count", "get_reverb_ray_count");

    ClassDB::bind_method(D_METHOD("get_reverb_bounce_count"), &VASource::get_reverb_bounce_count);
    ClassDB::bind_method(D_METHOD("set_reverb_bounce_count", "value"), &VASource::set_reverb_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_bounce_count"), "set_reverb_bounce_count", "get_reverb_bounce_count");

    ClassDB::bind_method(D_METHOD("get_reverb_energy_cap"), &VASource::get_reverb_energy_cap);
    ClassDB::bind_method(D_METHOD("set_reverb_energy_cap", "value"), &VASource::set_reverb_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reverb_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_reverb_energy_cap", "get_reverb_energy_cap");

    ClassDB::bind_method(D_METHOD("get_max_volume"), &VASource::get_max_volume);
    ClassDB::bind_method(D_METHOD("set_max_volume", "value"), &VASource::set_max_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_volume", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_max_volume", "get_max_volume");

    ClassDB::bind_method(D_METHOD("get_max_echogram_time"), &VASource::get_max_echogram_time);
    ClassDB::bind_method(D_METHOD("set_max_echogram_time", "value"), &VASource::set_max_echogram_time);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_echogram_time"), "set_max_echogram_time", "get_max_echogram_time");

    ClassDB::bind_method(D_METHOD("get_echogram_granularity"), &VASource::get_echogram_granularity);
    ClassDB::bind_method(D_METHOD("set_echogram_granularity", "value"), &VASource::set_echogram_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "echogram_granularity"), "set_echogram_granularity", "get_echogram_granularity");

    ClassDB::bind_method(D_METHOD("get_affects_grouped_eax"), &VASource::get_affects_grouped_eax);
    ClassDB::bind_method(D_METHOD("set_affects_grouped_eax", "value"), &VASource::set_affects_grouped_eax);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "affects_grouped_eax"), "set_affects_grouped_eax", "get_affects_grouped_eax");

    ADD_GROUP("Muffling", "");

    ClassDB::bind_method(D_METHOD("get_occlusion_energy_cap"), &VASource::get_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_occlusion_energy_cap", "value"), &VASource::set_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_occlusion_energy_cap", "get_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_permeation_energy_cap"), &VASource::get_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_permeation_energy_cap", "value"), &VASource::set_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_permeation_energy_cap", "get_permeation_energy_cap");

    ADD_GROUP("Ambience", "");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_ray_count"), &VASource::get_ambient_occlusion_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_ray_count", "value"), &VASource::set_ambient_occlusion_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_ray_count"), "set_ambient_occlusion_ray_count", "get_ambient_occlusion_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_bounce_count"), &VASource::get_ambient_occlusion_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_bounce_count", "value"), &VASource::set_ambient_occlusion_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_bounce_count"), "set_ambient_occlusion_bounce_count", "get_ambient_occlusion_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_energy_cap"), &VASource::get_ambient_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_energy_cap", "value"), &VASource::set_ambient_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_occlusion_energy_cap", "get_ambient_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_ray_count"), &VASource::get_ambient_permeation_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_ray_count", "value"), &VASource::set_ambient_permeation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_ray_count"), "set_ambient_permeation_ray_count", "get_ambient_permeation_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_bounce_count"), &VASource::get_ambient_permeation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_bounce_count", "value"), &VASource::set_ambient_permeation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_bounce_count"), "set_ambient_permeation_bounce_count", "get_ambient_permeation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_energy_cap"), &VASource::get_ambient_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_energy_cap", "value"), &VASource::set_ambient_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_permeation_energy_cap", "get_ambient_permeation_energy_cap");

    ADD_GROUP("Visualisation", "");

    ClassDB::bind_method(D_METHOD("get_visualisation_ray_count"), &VASource::get_visualisation_ray_count);
    ClassDB::bind_method(D_METHOD("set_visualisation_ray_count", "value"), &VASource::set_visualisation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_ray_count"), "set_visualisation_ray_count", "get_visualisation_ray_count");

    ClassDB::bind_method(D_METHOD("get_visualisation_bounce_count"), &VASource::get_visualisation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_visualisation_bounce_count", "value"), &VASource::set_visualisation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_bounce_count"), "set_visualisation_bounce_count", "get_visualisation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_visualisation_update_frequency"), &VASource::get_visualisation_update_frequency);
    ClassDB::bind_method(D_METHOD("set_visualisation_update_frequency", "value"), &VASource::set_visualisation_update_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_update_frequency"), "set_visualisation_update_frequency", "get_visualisation_update_frequency");

    ADD_GROUP("Advanced", "");

    ClassDB::bind_method(D_METHOD("get_type"), &VASource::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "value"), &VASource::set_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type"), "set_type", "get_type");

    ClassDB::bind_method(D_METHOD("get_refresh_ray_count"), &VASource::get_refresh_ray_count);
    ClassDB::bind_method(D_METHOD("set_refresh_ray_count", "value"), &VASource::set_refresh_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "refresh_ray_count"), "set_refresh_ray_count", "get_refresh_ray_count");

    ClassDB::bind_method(D_METHOD("get_refresh_distance_threshold"), &VASource::get_refresh_distance_threshold);
    ClassDB::bind_method(D_METHOD("set_refresh_distance_threshold", "value"), &VASource::set_refresh_distance_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_distance_threshold"), "set_refresh_distance_threshold", "get_refresh_distance_threshold");

    ClassDB::bind_method(D_METHOD("get_scattering_seed"), &VASource::get_scattering_seed);
    ClassDB::bind_method(D_METHOD("set_scattering_seed", "value"), &VASource::set_scattering_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scattering_seed"), "set_scattering_seed", "get_scattering_seed");
}

VASource::VASource()
{
    // Matches VASourceProperties.cs's `Random.Shared.Next()` field
    // initializer (VASource's version omits the int.MaxValue upper bound
    // VAEmitterProperties.cs uses, but Godot's randi() is unsigned either
    // way, so both are masked down to a non-negative int the same way here).
    scattering_seed = (int)(UtilityFunctions::randi() & 0x7fffffff);
}

VASource::~VASource()
{
}

bool VASource::get_play_when_raytracing_completes() const
{
    return play_when_raytracing_completes;
}

void VASource::set_play_when_raytracing_completes(bool value)
{
    play_when_raytracing_completes = value;
}

float VASource::get_muffling_gain_lf() const
{
    return filter.get_gain();
}

float VASource::get_muffling_gain_hf() const
{
    return filter.get_gain_hf();
}

void VASource::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on
        // every future node addition instead of erroring out permanently -
        // see VAEmitter::_enter_tree's identical pattern for the rationale.
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VASource::retry_find_va_world));
        return;
    }

    create_emitter();
}

void VASource::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VASource::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VASource::retry_find_va_world));
        }

        waiting_for_world = false;

        // Never found a VAWorld anywhere in the tree for this node's entire
        // time in it - see VAEmitter::_exit_tree's identical warning.
        VA_WARN(
            "'", get_name(),
            "' left the tree without ever finding a VAWorld - "
            "no emitter was created for it. Make sure this node's scene "
            "was added under a VAWorld while it was in the tree.");
    }

    if (emitter)
    {
        // remove_child alone only detaches - VAEmitter is exclusively owned
        // by this VASource (not referenced elsewhere), so it must also be
        // freed here or it leaks (confirmed via a headless run reporting a
        // leaked "<name>-Emitter" ObjectDB instance before this fix).
        remove_child(emitter);
        memdelete(emitter);
        emitter = nullptr;
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree -
// see VAEmitter::retry_find_va_world's identical pattern.
void VASource::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VASource::retry_find_va_world));
    waiting_for_world = false;

    create_emitter();
}

// Matches VASource.cs's CreateEmitter: a private child VAEmitter, never the
// listener, that never casts its own occlusion/permeation rays (only reverb
// rays) - a VASource is heard via the listener's rays targeting it, not by
// casting its own muffling rays.
void VASource::create_emitter()
{
    emitter = memnew(va_godot::VAEmitter);
    emitter->set_name(String(get_name()) + "-Emitter");
    add_child(emitter);

    // Matches VASource.cs's CreateEmitter, which forces these onto the child
    // emitter regardless of what a user may have set in the inspector - a
    // VASource's own emitter only ever casts reverb rays; muffling is
    // something done to it by the listener's rays, not something it casts
    // itself. Routed through VAEmitter's own setters (rather than raw SDK
    // calls) now that it has a tuning-knob property surface, so the cached
    // C++-side state stays consistent with what's actually pushed to the SDK.
    emitter->set_has_relative_reverb(false);
    emitter->set_occlusion_ray_count(0);
    emitter->set_occlusion_bounce_count(0);
    emitter->set_permeation_ray_count(0);
    emitter->set_permeation_bounce_count(0);

    apply_properties_to_emitter();
}

void VASource::apply_properties_to_emitter()
{
    emitter->set_reverb_ray_count(reverb_ray_count);
    emitter->set_reverb_bounce_count(reverb_bounce_count);
    emitter->set_reverb_energy_cap(reverb_energy_cap);
    emitter->set_max_volume(max_volume);
    emitter->set_max_echogram_time(max_echogram_time);
    emitter->set_echogram_granularity(echogram_granularity);
    emitter->set_affects_grouped_eax(affects_grouped_eax);

    emitter->set_occlusion_energy_cap(occlusion_energy_cap);
    emitter->set_permeation_energy_cap(permeation_energy_cap);

    emitter->set_ambient_occlusion_ray_count(ambient_occlusion_ray_count);
    emitter->set_ambient_occlusion_bounce_count(ambient_occlusion_bounce_count);
    emitter->set_ambient_occlusion_energy_cap(ambient_occlusion_energy_cap);
    emitter->set_ambient_permeation_ray_count(ambient_permeation_ray_count);
    emitter->set_ambient_permeation_bounce_count(ambient_permeation_bounce_count);
    emitter->set_ambient_permeation_energy_cap(ambient_permeation_energy_cap);

    emitter->set_visualisation_ray_count(visualisation_ray_count);
    emitter->set_visualisation_bounce_count(visualisation_bounce_count);
    emitter->set_visualisation_update_frequency(visualisation_update_frequency);

    emitter->set_type(type);
    emitter->set_refresh_ray_count(refresh_ray_count);
    emitter->set_refresh_distance_threshold(refresh_distance_threshold);
    emitter->set_scattering_seed(scattering_seed);
}

bool VASource::is_raytraced() const
{
    return emitter && emitter->is_raytraced();
}

bool VASource::play()
{
    if (!is_raytraced())
    {
        play_when_raytracing_completes = true;
        return false;
    }

    played = ALSourceNode3D::play();

    return played;
}

void VASource::_process(double delta)
{
    ALSourceNode3D::_process(delta);

    if (!is_raytraced())
    {
        return;
    }

    if (!played && play_when_raytracing_completes)
    {
        play();
    }

    va_godot::VAEmitter *listener = va_world->get_listener();

    if (listener)
    {
        apply_raytracing_results(listener);
    }
}

// VASource.cs's ApplyRaytracingResults(vaudio.Emitter other) port: resolves
// this source's reverb slot via its own child emitter's
// AffectsGroupedEAX/GroupedEAXIndex (VAWorld::get_reverb_effect - the
// listener slot if this source doesn't cast reverb rays into a grouped
// zone), then - if the listener has raytraced this source's emitter as a
// target - pushes the resulting muffling gain with fullReverb=true (reverb
// send always gets the clean/unfiltered signal; only the direct path is
// muffled).
void VASource::apply_raytracing_results(va_godot::VAEmitter *other)
{
    effect = va_world->get_reverb_effect(emitter->get_handle());

    if (other->has_raytraced_target(emitter))
    {
        VALowPassFilter vaudio_filter = other->get_target_filter(emitter);
        update_filter(vaudio_filter.gainLF, vaudio_filter.gainHF, true);
    }
}

// Exported tuning-knob getter/setter bodies - direct port of
// VASourceProperties.cs, including its Math.Max(0, value) clamps. Each
// setter stores locally (so the value survives if set before create_emitter()
// runs) and forwards to the child VAEmitter's own setter once it exists,
// mirroring VAEmitter's own "if (emitter != null)" guard pattern.

int VASource::get_reverb_ray_count() const
{
    return reverb_ray_count;
}

void VASource::set_reverb_ray_count(int value)
{
    reverb_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_ray_count(reverb_ray_count);
    }
}

int VASource::get_reverb_bounce_count() const
{
    return reverb_bounce_count;
}

void VASource::set_reverb_bounce_count(int value)
{
    reverb_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_bounce_count(reverb_bounce_count);
    }
}

float VASource::get_reverb_energy_cap() const
{
    return reverb_energy_cap;
}

void VASource::set_reverb_energy_cap(float value)
{
    reverb_energy_cap = value;

    if (emitter)
    {
        emitter->set_reverb_energy_cap(reverb_energy_cap);
    }
}

float VASource::get_max_volume() const
{
    return max_volume;
}

void VASource::set_max_volume(float value)
{
    max_volume = value;

    if (emitter)
    {
        emitter->set_max_volume(max_volume);
    }
}

int VASource::get_max_echogram_time() const
{
    return max_echogram_time;
}

void VASource::set_max_echogram_time(int value)
{
    max_echogram_time = MAX(0, value);

    if (emitter)
    {
        emitter->set_max_echogram_time(max_echogram_time);
    }
}

int VASource::get_echogram_granularity() const
{
    return echogram_granularity;
}

void VASource::set_echogram_granularity(int value)
{
    echogram_granularity = MAX(0, value);

    if (emitter)
    {
        emitter->set_echogram_granularity(echogram_granularity);
    }
}

bool VASource::get_affects_grouped_eax() const
{
    return affects_grouped_eax;
}

void VASource::set_affects_grouped_eax(bool value)
{
    affects_grouped_eax = value;

    if (emitter)
    {
        emitter->set_affects_grouped_eax(affects_grouped_eax);
    }
}

float VASource::get_occlusion_energy_cap() const
{
    return occlusion_energy_cap;
}

void VASource::set_occlusion_energy_cap(float value)
{
    occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_occlusion_energy_cap(occlusion_energy_cap);
    }
}

float VASource::get_permeation_energy_cap() const
{
    return permeation_energy_cap;
}

void VASource::set_permeation_energy_cap(float value)
{
    permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_permeation_energy_cap(permeation_energy_cap);
    }
}

int VASource::get_ambient_occlusion_ray_count() const
{
    return ambient_occlusion_ray_count;
}

void VASource::set_ambient_occlusion_ray_count(int value)
{
    ambient_occlusion_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_ray_count(ambient_occlusion_ray_count);
    }
}

int VASource::get_ambient_occlusion_bounce_count() const
{
    return ambient_occlusion_bounce_count;
}

void VASource::set_ambient_occlusion_bounce_count(int value)
{
    ambient_occlusion_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_bounce_count(ambient_occlusion_bounce_count);
    }
}

float VASource::get_ambient_occlusion_energy_cap() const
{
    return ambient_occlusion_energy_cap;
}

void VASource::set_ambient_occlusion_energy_cap(float value)
{
    ambient_occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_energy_cap(ambient_occlusion_energy_cap);
    }
}

int VASource::get_ambient_permeation_ray_count() const
{
    return ambient_permeation_ray_count;
}

void VASource::set_ambient_permeation_ray_count(int value)
{
    ambient_permeation_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_ray_count(ambient_permeation_ray_count);
    }
}

int VASource::get_ambient_permeation_bounce_count() const
{
    return ambient_permeation_bounce_count;
}

void VASource::set_ambient_permeation_bounce_count(int value)
{
    ambient_permeation_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_bounce_count(ambient_permeation_bounce_count);
    }
}

float VASource::get_ambient_permeation_energy_cap() const
{
    return ambient_permeation_energy_cap;
}

void VASource::set_ambient_permeation_energy_cap(float value)
{
    ambient_permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_energy_cap(ambient_permeation_energy_cap);
    }
}

int VASource::get_visualisation_ray_count() const
{
    return visualisation_ray_count;
}

void VASource::set_visualisation_ray_count(int value)
{
    visualisation_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_visualisation_ray_count(visualisation_ray_count);
    }
}

int VASource::get_visualisation_bounce_count() const
{
    return visualisation_bounce_count;
}

void VASource::set_visualisation_bounce_count(int value)
{
    visualisation_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_visualisation_bounce_count(visualisation_bounce_count);
    }
}

int VASource::get_visualisation_update_frequency() const
{
    return visualisation_update_frequency;
}

void VASource::set_visualisation_update_frequency(int value)
{
    visualisation_update_frequency = MAX(0, value);

    if (emitter)
    {
        emitter->set_visualisation_update_frequency(visualisation_update_frequency);
    }
}

int VASource::get_type() const
{
    return type;
}

void VASource::set_type(int value)
{
    type = value;

    if (emitter)
    {
        emitter->set_type(type);
    }
}

int VASource::get_refresh_ray_count() const
{
    return refresh_ray_count;
}

void VASource::set_refresh_ray_count(int value)
{
    refresh_ray_count = value;

    if (emitter)
    {
        emitter->set_refresh_ray_count(refresh_ray_count);
    }
}

float VASource::get_refresh_distance_threshold() const
{
    return refresh_distance_threshold;
}

void VASource::set_refresh_distance_threshold(float value)
{
    refresh_distance_threshold = value;

    if (emitter)
    {
        emitter->set_refresh_distance_threshold(refresh_distance_threshold);
    }
}

int VASource::get_scattering_seed() const
{
    return scattering_seed;
}

void VASource::set_scattering_seed(int value)
{
    scattering_seed = value;

    if (emitter)
    {
        emitter->set_scattering_seed(scattering_seed);
    }
}
