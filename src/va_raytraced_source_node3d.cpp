#include "va_raytraced_source_node3d.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VARaytracedSourceNode3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_raytrace_once"), &VARaytracedSourceNode3D::get_raytrace_once);
    ClassDB::bind_method(D_METHOD("set_raytrace_once", "value"), &VARaytracedSourceNode3D::set_raytrace_once);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "raytrace_once"), "set_raytrace_once", "get_raytrace_once");

    // Read-only muffling stats
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VARaytracedSourceNode3D::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VARaytracedSourceNode3D::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VARaytracedSourceNode3D::is_raytraced);

    // Direct port of vaudio-godot-mono-openal-3d's VASourceProperties.cs groups
    // (Reverb/Muffling/Ambience/Advanced) - see the header for why this is a
    // subset of VAEmitter's own property surface. Debug Rendering colors are
    // not ported, same rationale as VAEmitter.
    ADD_GROUP("Reverb", "");

    ClassDB::bind_method(D_METHOD("get_reverb_ray_count"), &VARaytracedSourceNode3D::get_reverb_ray_count);
    ClassDB::bind_method(D_METHOD("set_reverb_ray_count", "value"), &VARaytracedSourceNode3D::set_reverb_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_ray_count"), "set_reverb_ray_count", "get_reverb_ray_count");

    ClassDB::bind_method(D_METHOD("get_reverb_bounce_count"), &VARaytracedSourceNode3D::get_reverb_bounce_count);
    ClassDB::bind_method(D_METHOD("set_reverb_bounce_count", "value"), &VARaytracedSourceNode3D::set_reverb_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_bounce_count"), "set_reverb_bounce_count", "get_reverb_bounce_count");

    ClassDB::bind_method(D_METHOD("get_reverb_energy_cap"), &VARaytracedSourceNode3D::get_reverb_energy_cap);
    ClassDB::bind_method(D_METHOD("set_reverb_energy_cap", "value"), &VARaytracedSourceNode3D::set_reverb_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reverb_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_reverb_energy_cap", "get_reverb_energy_cap");

    ClassDB::bind_method(D_METHOD("get_max_volume"), &VARaytracedSourceNode3D::get_max_volume);
    ClassDB::bind_method(D_METHOD("set_max_volume", "value"), &VARaytracedSourceNode3D::set_max_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_volume", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_max_volume", "get_max_volume");

    ClassDB::bind_method(D_METHOD("get_max_echogram_time"), &VARaytracedSourceNode3D::get_max_echogram_time);
    ClassDB::bind_method(D_METHOD("set_max_echogram_time", "value"), &VARaytracedSourceNode3D::set_max_echogram_time);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_echogram_time"), "set_max_echogram_time", "get_max_echogram_time");

    ClassDB::bind_method(D_METHOD("get_echogram_granularity"), &VARaytracedSourceNode3D::get_echogram_granularity);
    ClassDB::bind_method(D_METHOD("set_echogram_granularity", "value"), &VARaytracedSourceNode3D::set_echogram_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "echogram_granularity"), "set_echogram_granularity", "get_echogram_granularity");

    ClassDB::bind_method(D_METHOD("get_affects_grouped_eax"), &VARaytracedSourceNode3D::get_affects_grouped_eax);
    ClassDB::bind_method(D_METHOD("set_affects_grouped_eax", "value"), &VARaytracedSourceNode3D::set_affects_grouped_eax);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "affects_grouped_eax"), "set_affects_grouped_eax", "get_affects_grouped_eax");

    ClassDB::bind_method(D_METHOD("get_use_listener_reverb"), &VARaytracedSourceNode3D::get_use_listener_reverb);
    ClassDB::bind_method(D_METHOD("set_use_listener_reverb", "value"), &VARaytracedSourceNode3D::set_use_listener_reverb);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_listener_reverb"), "set_use_listener_reverb", "get_use_listener_reverb");

    ADD_GROUP("Muffling", "");

    ClassDB::bind_method(D_METHOD("get_occlusion_energy_cap"), &VARaytracedSourceNode3D::get_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_occlusion_energy_cap", "value"), &VARaytracedSourceNode3D::set_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_occlusion_energy_cap", "get_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_permeation_energy_cap"), &VARaytracedSourceNode3D::get_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_permeation_energy_cap", "value"), &VARaytracedSourceNode3D::set_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_permeation_energy_cap", "get_permeation_energy_cap");

    ADD_GROUP("Ambience", "");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_ray_count"), &VARaytracedSourceNode3D::get_ambient_occlusion_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_ray_count", "value"), &VARaytracedSourceNode3D::set_ambient_occlusion_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_ray_count"), "set_ambient_occlusion_ray_count", "get_ambient_occlusion_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_bounce_count"), &VARaytracedSourceNode3D::get_ambient_occlusion_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_bounce_count", "value"), &VARaytracedSourceNode3D::set_ambient_occlusion_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_bounce_count"), "set_ambient_occlusion_bounce_count", "get_ambient_occlusion_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_energy_cap"), &VARaytracedSourceNode3D::get_ambient_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_energy_cap", "value"), &VARaytracedSourceNode3D::set_ambient_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_occlusion_energy_cap", "get_ambient_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_ray_count"), &VARaytracedSourceNode3D::get_ambient_permeation_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_ray_count", "value"), &VARaytracedSourceNode3D::set_ambient_permeation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_ray_count"), "set_ambient_permeation_ray_count", "get_ambient_permeation_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_bounce_count"), &VARaytracedSourceNode3D::get_ambient_permeation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_bounce_count", "value"), &VARaytracedSourceNode3D::set_ambient_permeation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_bounce_count"), "set_ambient_permeation_bounce_count", "get_ambient_permeation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_energy_cap"), &VARaytracedSourceNode3D::get_ambient_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_energy_cap", "value"), &VARaytracedSourceNode3D::set_ambient_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_permeation_energy_cap", "get_ambient_permeation_energy_cap");

    ADD_GROUP("Advanced", "");

    ClassDB::bind_method(D_METHOD("get_type"), &VARaytracedSourceNode3D::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "value"), &VARaytracedSourceNode3D::set_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type"), "set_type", "get_type");

    ClassDB::bind_method(D_METHOD("get_refresh_ray_count"), &VARaytracedSourceNode3D::get_refresh_ray_count);
    ClassDB::bind_method(D_METHOD("set_refresh_ray_count", "value"), &VARaytracedSourceNode3D::set_refresh_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "refresh_ray_count"), "set_refresh_ray_count", "get_refresh_ray_count");

    ClassDB::bind_method(D_METHOD("get_refresh_distance_threshold"), &VARaytracedSourceNode3D::get_refresh_distance_threshold);
    ClassDB::bind_method(D_METHOD("set_refresh_distance_threshold", "value"), &VARaytracedSourceNode3D::set_refresh_distance_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_distance_threshold"), "set_refresh_distance_threshold", "get_refresh_distance_threshold");

    ClassDB::bind_method(D_METHOD("get_scattering_seed"), &VARaytracedSourceNode3D::get_scattering_seed);
    ClassDB::bind_method(D_METHOD("set_scattering_seed", "value"), &VARaytracedSourceNode3D::set_scattering_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scattering_seed"), "set_scattering_seed", "get_scattering_seed");
}

VARaytracedSourceNode3D::VARaytracedSourceNode3D()
{
    // Matches VASourceProperties.cs's `Random.Shared.Next()` field
    // initializer (this omits the int.MaxValue upper bound
    // VAEmitterProperties.cs uses, but Godot's randi() is unsigned either
    // way, so both are masked down to a non-negative int the same way here).
    scattering_seed = (int)(UtilityFunctions::randi() & 0x7fffffff);
}

VARaytracedSourceNode3D::~VARaytracedSourceNode3D()
{
}

bool VARaytracedSourceNode3D::get_raytrace_once() const
{
    return raytrace_once;
}

void VARaytracedSourceNode3D::set_raytrace_once(bool value)
{
    raytrace_once = value;

    if (emitter)
    {
        emitter->set_raytrace_once(raytrace_once);
    }
}

float VARaytracedSourceNode3D::get_muffling_gain_lf() const
{
    return filter.get_gain();
}

float VARaytracedSourceNode3D::get_muffling_gain_hf() const
{
    return filter.get_gain_hf();
}

void VARaytracedSourceNode3D::_enter_tree()
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
        get_tree()->connect("node_added", callable_mp(this, &VARaytracedSourceNode3D::retry_find_va_world));
        return;
    }

    create_emitter();
}

void VARaytracedSourceNode3D::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VARaytracedSourceNode3D::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VARaytracedSourceNode3D::retry_find_va_world));
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
        // by this node (not referenced elsewhere), so it must also be freed
        // here or it leaks (confirmed via a headless run reporting a leaked
        // "<name>-Emitter" ObjectDB instance before this fix).
        remove_child(emitter);
        memdelete(emitter);
        emitter = nullptr;
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree -
// see VAEmitter::retry_find_va_world's identical pattern.
void VARaytracedSourceNode3D::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VARaytracedSourceNode3D::retry_find_va_world));
    waiting_for_world = false;

    create_emitter();
}

// Matches VASource.cs's CreateEmitter: a private child VAEmitter, never the
// listener, that never casts its own occlusion/permeation rays (only reverb
// rays) - this node is heard via the listener's rays targeting it, not by
// casting its own muffling rays.
void VARaytracedSourceNode3D::create_emitter()
{
    emitter = memnew(va_godot::VAEmitter);
    emitter->set_name(String(get_name()) + "-Emitter");
    add_child(emitter);

    // Matches VASource.cs's CreateEmitter, which forces these onto the child
    // emitter regardless of what a user may have set in the inspector - this
    // node's own emitter only ever casts reverb rays; muffling is something
    // done to it by the listener's rays, not something it casts itself.
    // Routed through VAEmitter's own setters (rather than raw SDK calls) now
    // that it has a tuning-knob property surface, so the cached C++-side
    // state stays consistent with what's actually pushed to the SDK.
    emitter->set_has_relative_reverb(false);
    emitter->set_occlusion_ray_count(0);
    emitter->set_occlusion_bounce_count(0);
    emitter->set_permeation_ray_count(0);
    emitter->set_permeation_bounce_count(0);
    emitter->set_raytrace_once(raytrace_once);

    apply_properties_to_emitter();
}

void VARaytracedSourceNode3D::apply_properties_to_emitter()
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

bool VARaytracedSourceNode3D::is_raytraced() const
{
    return emitter && emitter->is_raytraced();
}

void VARaytracedSourceNode3D::process_raytracing(double delta)
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

// VASource.cs's ApplyRaytracingResults(vaudio.Emitter other) port: resolves
// this node's reverb slot via its own child emitter's
// AffectsGroupedEAX/GroupedEAXIndex (VAWorld::get_reverb_effect - the
// listener slot if this node doesn't cast reverb rays into a grouped zone),
// then - if the listener has raytraced this node's emitter as a target -
// pushes the resulting muffling gain with fullReverb=true (reverb send
// always gets the clean/unfiltered signal; only the direct path is muffled).
void VARaytracedSourceNode3D::apply_raytracing_results(va_godot::VAEmitter *other)
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

int VARaytracedSourceNode3D::get_reverb_ray_count() const
{
    return reverb_ray_count;
}

void VARaytracedSourceNode3D::set_reverb_ray_count(int value)
{
    reverb_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_ray_count(reverb_ray_count);
    }
}

int VARaytracedSourceNode3D::get_reverb_bounce_count() const
{
    return reverb_bounce_count;
}

void VARaytracedSourceNode3D::set_reverb_bounce_count(int value)
{
    reverb_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_reverb_bounce_count(reverb_bounce_count);
    }
}

float VARaytracedSourceNode3D::get_reverb_energy_cap() const
{
    return reverb_energy_cap;
}

void VARaytracedSourceNode3D::set_reverb_energy_cap(float value)
{
    reverb_energy_cap = value;

    if (emitter)
    {
        emitter->set_reverb_energy_cap(reverb_energy_cap);
    }
}

float VARaytracedSourceNode3D::get_max_volume() const
{
    return max_volume;
}

void VARaytracedSourceNode3D::set_max_volume(float value)
{
    max_volume = value;

    if (emitter)
    {
        emitter->set_max_volume(max_volume);
    }
}

int VARaytracedSourceNode3D::get_max_echogram_time() const
{
    return max_echogram_time;
}

void VARaytracedSourceNode3D::set_max_echogram_time(int value)
{
    max_echogram_time = MAX(0, value);

    if (emitter)
    {
        emitter->set_max_echogram_time(max_echogram_time);
    }
}

int VARaytracedSourceNode3D::get_echogram_granularity() const
{
    return echogram_granularity;
}

void VARaytracedSourceNode3D::set_echogram_granularity(int value)
{
    echogram_granularity = MAX(0, value);

    if (emitter)
    {
        emitter->set_echogram_granularity(echogram_granularity);
    }
}

bool VARaytracedSourceNode3D::get_affects_grouped_eax() const
{
    return affects_grouped_eax;
}

void VARaytracedSourceNode3D::set_affects_grouped_eax(bool value)
{
    affects_grouped_eax = value;

    if (emitter)
    {
        emitter->set_affects_grouped_eax(affects_grouped_eax);
    }
}

bool VARaytracedSourceNode3D::get_use_listener_reverb() const
{
    return use_listener_reverb;
}

void VARaytracedSourceNode3D::set_use_listener_reverb(bool value)
{
    use_listener_reverb = value;

    if (emitter)
    {
        emitter->set_use_listener_reverb(use_listener_reverb);
    }
}

float VARaytracedSourceNode3D::get_occlusion_energy_cap() const
{
    return occlusion_energy_cap;
}

void VARaytracedSourceNode3D::set_occlusion_energy_cap(float value)
{
    occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_occlusion_energy_cap(occlusion_energy_cap);
    }
}

float VARaytracedSourceNode3D::get_permeation_energy_cap() const
{
    return permeation_energy_cap;
}

void VARaytracedSourceNode3D::set_permeation_energy_cap(float value)
{
    permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_permeation_energy_cap(permeation_energy_cap);
    }
}

int VARaytracedSourceNode3D::get_ambient_occlusion_ray_count() const
{
    return ambient_occlusion_ray_count;
}

void VARaytracedSourceNode3D::set_ambient_occlusion_ray_count(int value)
{
    ambient_occlusion_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_ray_count(ambient_occlusion_ray_count);
    }
}

int VARaytracedSourceNode3D::get_ambient_occlusion_bounce_count() const
{
    return ambient_occlusion_bounce_count;
}

void VARaytracedSourceNode3D::set_ambient_occlusion_bounce_count(int value)
{
    ambient_occlusion_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_bounce_count(ambient_occlusion_bounce_count);
    }
}

float VARaytracedSourceNode3D::get_ambient_occlusion_energy_cap() const
{
    return ambient_occlusion_energy_cap;
}

void VARaytracedSourceNode3D::set_ambient_occlusion_energy_cap(float value)
{
    ambient_occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_occlusion_energy_cap(ambient_occlusion_energy_cap);
    }
}

int VARaytracedSourceNode3D::get_ambient_permeation_ray_count() const
{
    return ambient_permeation_ray_count;
}

void VARaytracedSourceNode3D::set_ambient_permeation_ray_count(int value)
{
    ambient_permeation_ray_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_ray_count(ambient_permeation_ray_count);
    }
}

int VARaytracedSourceNode3D::get_ambient_permeation_bounce_count() const
{
    return ambient_permeation_bounce_count;
}

void VARaytracedSourceNode3D::set_ambient_permeation_bounce_count(int value)
{
    ambient_permeation_bounce_count = MAX(0, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_bounce_count(ambient_permeation_bounce_count);
    }
}

float VARaytracedSourceNode3D::get_ambient_permeation_energy_cap() const
{
    return ambient_permeation_energy_cap;
}

void VARaytracedSourceNode3D::set_ambient_permeation_energy_cap(float value)
{
    ambient_permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        emitter->set_ambient_permeation_energy_cap(ambient_permeation_energy_cap);
    }
}

int VARaytracedSourceNode3D::get_type() const
{
    return type;
}

void VARaytracedSourceNode3D::set_type(int value)
{
    type = value;

    if (emitter)
    {
        emitter->set_type(type);
    }
}

int VARaytracedSourceNode3D::get_refresh_ray_count() const
{
    return refresh_ray_count;
}

void VARaytracedSourceNode3D::set_refresh_ray_count(int value)
{
    refresh_ray_count = value;

    if (emitter)
    {
        emitter->set_refresh_ray_count(refresh_ray_count);
    }
}

float VARaytracedSourceNode3D::get_refresh_distance_threshold() const
{
    return refresh_distance_threshold;
}

void VARaytracedSourceNode3D::set_refresh_distance_threshold(float value)
{
    refresh_distance_threshold = value;

    if (emitter)
    {
        emitter->set_refresh_distance_threshold(refresh_distance_threshold);
    }
}

int VARaytracedSourceNode3D::get_scattering_seed() const
{
    return scattering_seed;
}

void VARaytracedSourceNode3D::set_scattering_seed(int value)
{
    scattering_seed = value;

    if (emitter)
    {
        emitter->set_scattering_seed(scattering_seed);
    }
}
