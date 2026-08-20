#include "va_raytraced_source.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VARaytracedSource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_raytrace_once"), &VARaytracedSource::get_raytrace_once);
    ClassDB::bind_method(D_METHOD("set_raytrace_once", "value"), &VARaytracedSource::set_raytrace_once);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "raytrace_once"), "set_raytrace_once", "get_raytrace_once");

    // Read-only muffling stats
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VARaytracedSource::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VARaytracedSource::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VARaytracedSource::is_raytraced);

    // Direct port of VASourceProperties.cs's groups (Reverb/Muffling/Ambience/Advanced) - a subset of VAEmitter's own property surface; Debug Rendering colors not ported, same as VAEmitter.
    ADD_GROUP("Reverb", "");

    ClassDB::bind_method(D_METHOD("get_reverb_ray_count"), &VARaytracedSource::get_reverb_ray_count);
    ClassDB::bind_method(D_METHOD("set_reverb_ray_count", "value"), &VARaytracedSource::set_reverb_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_ray_count"), "set_reverb_ray_count", "get_reverb_ray_count");

    ClassDB::bind_method(D_METHOD("get_reverb_bounce_count"), &VARaytracedSource::get_reverb_bounce_count);
    ClassDB::bind_method(D_METHOD("set_reverb_bounce_count", "value"), &VARaytracedSource::set_reverb_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_bounce_count"), "set_reverb_bounce_count", "get_reverb_bounce_count");

    ClassDB::bind_method(D_METHOD("get_reverb_energy_cap"), &VARaytracedSource::get_reverb_energy_cap);
    ClassDB::bind_method(D_METHOD("set_reverb_energy_cap", "value"), &VARaytracedSource::set_reverb_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reverb_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_reverb_energy_cap", "get_reverb_energy_cap");

    ClassDB::bind_method(D_METHOD("get_max_volume"), &VARaytracedSource::get_max_volume);
    ClassDB::bind_method(D_METHOD("set_max_volume", "value"), &VARaytracedSource::set_max_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_volume", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_max_volume", "get_max_volume");

    ClassDB::bind_method(D_METHOD("get_max_echogram_time"), &VARaytracedSource::get_max_echogram_time);
    ClassDB::bind_method(D_METHOD("set_max_echogram_time", "value"), &VARaytracedSource::set_max_echogram_time);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_echogram_time"), "set_max_echogram_time", "get_max_echogram_time");

    ClassDB::bind_method(D_METHOD("get_echogram_granularity"), &VARaytracedSource::get_echogram_granularity);
    ClassDB::bind_method(D_METHOD("set_echogram_granularity", "value"), &VARaytracedSource::set_echogram_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "echogram_granularity"), "set_echogram_granularity", "get_echogram_granularity");

    ClassDB::bind_method(D_METHOD("get_affects_grouped_eax"), &VARaytracedSource::get_affects_grouped_eax);
    ClassDB::bind_method(D_METHOD("set_affects_grouped_eax", "value"), &VARaytracedSource::set_affects_grouped_eax);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "affects_grouped_eax"), "set_affects_grouped_eax", "get_affects_grouped_eax");

    ClassDB::bind_method(D_METHOD("get_use_listener_reverb"), &VARaytracedSource::get_use_listener_reverb);
    ClassDB::bind_method(D_METHOD("set_use_listener_reverb", "value"), &VARaytracedSource::set_use_listener_reverb);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_listener_reverb"), "set_use_listener_reverb", "get_use_listener_reverb");

    ADD_GROUP("Muffling", "");

    ClassDB::bind_method(D_METHOD("get_occlusion_energy_cap"), &VARaytracedSource::get_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_occlusion_energy_cap", "value"), &VARaytracedSource::set_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_occlusion_energy_cap", "get_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_permeation_energy_cap"), &VARaytracedSource::get_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_permeation_energy_cap", "value"), &VARaytracedSource::set_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_permeation_energy_cap", "get_permeation_energy_cap");

    ADD_GROUP("Ambience", "");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_ray_count"), &VARaytracedSource::get_ambient_occlusion_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_ray_count", "value"), &VARaytracedSource::set_ambient_occlusion_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_ray_count"), "set_ambient_occlusion_ray_count", "get_ambient_occlusion_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_bounce_count"), &VARaytracedSource::get_ambient_occlusion_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_bounce_count", "value"), &VARaytracedSource::set_ambient_occlusion_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_bounce_count"), "set_ambient_occlusion_bounce_count", "get_ambient_occlusion_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_energy_cap"), &VARaytracedSource::get_ambient_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_energy_cap", "value"), &VARaytracedSource::set_ambient_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_occlusion_energy_cap", "get_ambient_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_ray_count"), &VARaytracedSource::get_ambient_permeation_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_ray_count", "value"), &VARaytracedSource::set_ambient_permeation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_ray_count"), "set_ambient_permeation_ray_count", "get_ambient_permeation_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_bounce_count"), &VARaytracedSource::get_ambient_permeation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_bounce_count", "value"), &VARaytracedSource::set_ambient_permeation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_bounce_count"), "set_ambient_permeation_bounce_count", "get_ambient_permeation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_energy_cap"), &VARaytracedSource::get_ambient_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_energy_cap", "value"), &VARaytracedSource::set_ambient_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_permeation_energy_cap", "get_ambient_permeation_energy_cap");

    ADD_GROUP("Advanced", "");

    ClassDB::bind_method(D_METHOD("get_type"), &VARaytracedSource::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "value"), &VARaytracedSource::set_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type"), "set_type", "get_type");

    ClassDB::bind_method(D_METHOD("get_refresh_ray_count"), &VARaytracedSource::get_refresh_ray_count);
    ClassDB::bind_method(D_METHOD("set_refresh_ray_count", "value"), &VARaytracedSource::set_refresh_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "refresh_ray_count"), "set_refresh_ray_count", "get_refresh_ray_count");

    ClassDB::bind_method(D_METHOD("get_refresh_distance_threshold"), &VARaytracedSource::get_refresh_distance_threshold);
    ClassDB::bind_method(D_METHOD("set_refresh_distance_threshold", "value"), &VARaytracedSource::set_refresh_distance_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_distance_threshold"), "set_refresh_distance_threshold", "get_refresh_distance_threshold");

    ClassDB::bind_method(D_METHOD("get_scattering_seed"), &VARaytracedSource::get_scattering_seed);
    ClassDB::bind_method(D_METHOD("set_scattering_seed", "value"), &VARaytracedSource::set_scattering_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scattering_seed"), "set_scattering_seed", "get_scattering_seed");
}

VARaytracedSource::VARaytracedSource()
{
    // Matches VASourceProperties.cs's `Random.Shared.Next()` initializer; Godot's randi() is unsigned, so it's masked down to a non-negative int.
    scattering_seed = (int)(UtilityFunctions::randi() & 0x7fffffff);
}

VARaytracedSource::~VARaytracedSource()
{
}

bool VARaytracedSource::get_raytrace_once() const
{
    return raytrace_once;
}

void VARaytracedSource::set_raytrace_once(bool value)
{
    raytrace_once = value;

    if (emitter)
    {
        emitter->set_raytrace_once(raytrace_once);
    }
}

float VARaytracedSource::get_muffling_gain_lf() const
{
    return filter.get_gain();
}

float VARaytracedSource::get_muffling_gain_hf() const
{
    return filter.get_gain_hf();
}

void VARaytracedSource::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on every future node addition, see VAEmitter::_enter_tree's identical pattern.
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VARaytracedSource::retry_find_va_world));
        return;
    }

    create_emitter();
}

void VARaytracedSource::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VARaytracedSource::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VARaytracedSource::retry_find_va_world));
        }

        waiting_for_world = false;

        // Never found a VAWorld anywhere in the tree for this node's entire time in it - see VAEmitter::_exit_tree's identical warning.
        VA_WARN(
            "'", get_name(),
            "' left the tree without ever finding a VAWorld - "
            "no emitter was created for it. Make sure this node's scene "
            "was added under a VAWorld while it was in the tree.");
    }

    if (emitter)
    {
        // remove_child alone only detaches - VAEmitter is exclusively owned by this node, so it must also be freed here or it leaks (confirmed via a headless run reporting a leaked ObjectDB instance before this fix).
        remove_child(emitter);
        memdelete(emitter);
        emitter = nullptr;
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree - see VAEmitter::retry_find_va_world's identical pattern.
void VARaytracedSource::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VARaytracedSource::retry_find_va_world));
    waiting_for_world = false;

    create_emitter();
}

// Matches VASource.cs's CreateEmitter: a private child VAEmitter, never the listener, that never casts its own
// occlusion/permeation rays (only reverb rays) - this node is heard via the listener's rays targeting it, not its own.
void VARaytracedSource::create_emitter()
{
    emitter = memnew(va_godot::VAEmitter);
    emitter->set_name(String(get_name()) + "-Emitter");
    add_child(emitter);

    // Forces these onto the child emitter regardless of inspector settings - this emitter only ever casts reverb rays.
    // Routed through VAEmitter's own setters so the cached C++-side state stays consistent with what's pushed to the SDK.
    emitter->set_has_relative_reverb(false);
    emitter->set_occlusion_ray_count(0);
    emitter->set_occlusion_bounce_count(0);
    emitter->set_permeation_ray_count(0);
    emitter->set_permeation_bounce_count(0);
    emitter->set_raytrace_once(raytrace_once);

    apply_properties_to_emitter();
}

void VARaytracedSource::apply_properties_to_emitter()
{
    emitter->set_reverb_ray_count(reverb_ray_count);
    emitter->set_reverb_bounce_count(reverb_bounce_count);
    emitter->set_reverb_energy_cap(reverb_energy_cap);
    emitter->set_max_volume(max_volume);
    emitter->set_max_echogram_time(max_echogram_time);
    emitter->set_echogram_granularity(echogram_granularity);
    emitter->set_affects_grouped_eax(affects_grouped_eax);
    emitter->set_use_listener_reverb(use_listener_reverb);

    emitter->set_occlusion_energy_cap(occlusion_energy_cap);
    emitter->set_permeation_energy_cap(permeation_energy_cap);

    emitter->set_ambient_occlusion_ray_count(ambient_occlusion_ray_count);
    emitter->set_ambient_occlusion_bounce_count(ambient_occlusion_bounce_count);
    emitter->set_ambient_occlusion_energy_cap(ambient_occlusion_energy_cap);
    emitter->set_ambient_permeation_ray_count(ambient_permeation_ray_count);
    emitter->set_ambient_permeation_bounce_count(ambient_permeation_bounce_count);
    emitter->set_ambient_permeation_energy_cap(ambient_permeation_energy_cap);

    emitter->set_type(type);
    emitter->set_refresh_ray_count(refresh_ray_count);
    emitter->set_refresh_distance_threshold(refresh_distance_threshold);
    emitter->set_scattering_seed(scattering_seed);
}

bool VARaytracedSource::is_raytraced() const
{
    return emitter && emitter->is_raytraced();
}

void VARaytracedSource::process_raytracing(double delta)
{
    if (!is_raytraced())
    {
        return;
    }

    va_godot::VAEmitter *listener = va_world->get_listener();

    if (listener)
    {
        apply_raytracing_results(listener);
    }
}

// VASource.cs's ApplyRaytracingResults port: resolves this node's reverb slot via its child emitter's AffectsGroupedEAX/
// GroupedEAXIndex, then if the listener has raytraced this emitter as a target, pushes the muffling gain with fullReverb=true
// (reverb send always gets the clean/unfiltered signal; only the direct path is muffled).
void VARaytracedSource::apply_raytracing_results(va_godot::VAEmitter *other)
{
    effect = va_world->get_reverb_effect(emitter->get_handle());

    if (other->has_raytraced_target(emitter))
    {
        VALowPassFilter vaudio_filter = other->get_target_filter(emitter);
        update_filter(vaudio_filter.gainLF, vaudio_filter.gainHF, true);
    }
}

// Exported tuning-knob getter/setter bodies - direct port of VASourceProperties.cs including its clamps. Each setter stores
// locally (so the value survives if set before create_emitter() runs) and forwards to the child VAEmitter once it exists.

int VARaytracedSource::get_reverb_ray_count() const
{
    return reverb_ray_count;
}

void VARaytracedSource::set_reverb_ray_count(int value)
{
    reverb_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_ray_count(reverb_ray_count);
    }
}

int VARaytracedSource::get_reverb_bounce_count() const
{
    return reverb_bounce_count;
}

void VARaytracedSource::set_reverb_bounce_count(int value)
{
    reverb_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_bounce_count(reverb_bounce_count);
    }
}

float VARaytracedSource::get_reverb_energy_cap() const
{
    return reverb_energy_cap;
}

void VARaytracedSource::set_reverb_energy_cap(float value)
{
    reverb_energy_cap = value;

    if (emitter)
    {
        emitter->set_reverb_energy_cap(reverb_energy_cap);
    }
}

float VARaytracedSource::get_max_volume() const
{
    return max_volume;
}

void VARaytracedSource::set_max_volume(float value)
{
    max_volume = value;

    if (emitter)
    {
        emitter->set_max_volume(max_volume);
    }
}

int VARaytracedSource::get_max_echogram_time() const
{
    return max_echogram_time;
}

void VARaytracedSource::set_max_echogram_time(int value)
{
    max_echogram_time = MAX(0, value);

    if (emitter)
    {
        emitter->set_max_echogram_time(max_echogram_time);
    }
}

int VARaytracedSource::get_echogram_granularity() const
{
    return echogram_granularity;
}

void VARaytracedSource::set_echogram_granularity(int value)
{
    echogram_granularity = MAX(0, value);

    if (emitter)
    {
        emitter->set_echogram_granularity(echogram_granularity);
    }
}

bool VARaytracedSource::get_affects_grouped_eax() const
{
    return affects_grouped_eax;
}

void VARaytracedSource::set_affects_grouped_eax(bool value)
{
    affects_grouped_eax = value;

    if (emitter)
    {
        emitter->set_affects_grouped_eax(affects_grouped_eax);
    }
}

bool VARaytracedSource::get_use_listener_reverb() const
{
    return use_listener_reverb;
}

void VARaytracedSource::set_use_listener_reverb(bool value)
{
    use_listener_reverb = value;

    if (emitter)
    {
        emitter->set_use_listener_reverb(use_listener_reverb);
    }
}

float VARaytracedSource::get_occlusion_energy_cap() const
{
    return occlusion_energy_cap;
}

void VARaytracedSource::set_occlusion_energy_cap(float value)
{
    occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_occlusion_energy_cap(occlusion_energy_cap);
    }
}

float VARaytracedSource::get_permeation_energy_cap() const
{
    return permeation_energy_cap;
}

void VARaytracedSource::set_permeation_energy_cap(float value)
{
    permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_permeation_energy_cap(permeation_energy_cap);
    }
}

int VARaytracedSource::get_ambient_occlusion_ray_count() const
{
    return ambient_occlusion_ray_count;
}

void VARaytracedSource::set_ambient_occlusion_ray_count(int value)
{
    ambient_occlusion_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_ray_count(ambient_occlusion_ray_count);
    }
}

int VARaytracedSource::get_ambient_occlusion_bounce_count() const
{
    return ambient_occlusion_bounce_count;
}

void VARaytracedSource::set_ambient_occlusion_bounce_count(int value)
{
    ambient_occlusion_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_bounce_count(ambient_occlusion_bounce_count);
    }
}

float VARaytracedSource::get_ambient_occlusion_energy_cap() const
{
    return ambient_occlusion_energy_cap;
}

void VARaytracedSource::set_ambient_occlusion_energy_cap(float value)
{
    ambient_occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_energy_cap(ambient_occlusion_energy_cap);
    }
}

int VARaytracedSource::get_ambient_permeation_ray_count() const
{
    return ambient_permeation_ray_count;
}

void VARaytracedSource::set_ambient_permeation_ray_count(int value)
{
    ambient_permeation_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_ray_count(ambient_permeation_ray_count);
    }
}

int VARaytracedSource::get_ambient_permeation_bounce_count() const
{
    return ambient_permeation_bounce_count;
}

void VARaytracedSource::set_ambient_permeation_bounce_count(int value)
{
    ambient_permeation_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_bounce_count(ambient_permeation_bounce_count);
    }
}

float VARaytracedSource::get_ambient_permeation_energy_cap() const
{
    return ambient_permeation_energy_cap;
}

void VARaytracedSource::set_ambient_permeation_energy_cap(float value)
{
    ambient_permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_energy_cap(ambient_permeation_energy_cap);
    }
}

int VARaytracedSource::get_type() const
{
    return type;
}

void VARaytracedSource::set_type(int value)
{
    type = value;

    if (emitter)
    {
        emitter->set_type(type);
    }
}

int VARaytracedSource::get_refresh_ray_count() const
{
    return refresh_ray_count;
}

void VARaytracedSource::set_refresh_ray_count(int value)
{
    refresh_ray_count = value;

    if (emitter)
    {
        emitter->set_refresh_ray_count(refresh_ray_count);
    }
}

float VARaytracedSource::get_refresh_distance_threshold() const
{
    return refresh_distance_threshold;
}

void VARaytracedSource::set_refresh_distance_threshold(float value)
{
    refresh_distance_threshold = value;

    if (emitter)
    {
        emitter->set_refresh_distance_threshold(refresh_distance_threshold);
    }
}

int VARaytracedSource::get_scattering_seed() const
{
    return scattering_seed;
}

void VARaytracedSource::set_scattering_seed(int value)
{
    scattering_seed = value;

    if (emitter)
    {
        emitter->set_scattering_seed(scattering_seed);
    }
}
