#include "va_emitter.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_filter.h"
#include "openal/al_reverb.h"
#include "va_conversions.h"
#include "va_world.h"

namespace va_godot
{

// Finds the (singleton) VAWorld under the current scene root's children -
// same helper as VACustomMaterial's find_va_world (NodeExtensions.cs's
// GetVAWorldParent port). Duplicated locally rather than shared since it's a
// two-line static helper and each caller only needs it during _enter_tree.
static VAWorld *find_va_world(Node *node)
{
    Node *scene_root = node->get_tree()->get_current_scene();
    if (!scene_root)
    {
        return nullptr;
    }

    TypedArray<Node> children = scene_root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        if (VAWorld *world = Object::cast_to<VAWorld>(children[i]))
        {
            return world;
        }
    }

    return nullptr;
}

void VAEmitter::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_is_main_listener"), &VAEmitter::get_is_main_listener);
    ClassDB::bind_method(D_METHOD("set_is_main_listener", "value"), &VAEmitter::set_is_main_listener);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_main_listener"), "set_is_main_listener", "get_is_main_listener");

    // Read-only SDK forwards (not ADD_PROPERTY'd - called directly from
    // GDScript like methods, e.g. emitter.get_va_position()).
    ClassDB::bind_method(D_METHOD("get_va_position"), &VAEmitter::get_va_position);
    ClassDB::bind_method(D_METHOD("get_within_world_bounds"), &VAEmitter::get_within_world_bounds);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VAEmitter::is_raytraced);

    // Direct port of vaudio-godot-openal's VAEmitterProperties.cs groups
    // (Reverb/Muffling/Ambience/Visualisation/Advanced). Debug Rendering
    // colors are not ported - no backing SDK API in this C SDK version (see
    // va_emitter.h).
    ADD_GROUP("Reverb", "");

    ClassDB::bind_method(D_METHOD("get_reverb_ray_count"), &VAEmitter::get_reverb_ray_count);
    ClassDB::bind_method(D_METHOD("set_reverb_ray_count", "value"), &VAEmitter::set_reverb_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_ray_count"), "set_reverb_ray_count", "get_reverb_ray_count");

    ClassDB::bind_method(D_METHOD("get_reverb_bounce_count"), &VAEmitter::get_reverb_bounce_count);
    ClassDB::bind_method(D_METHOD("set_reverb_bounce_count", "value"), &VAEmitter::set_reverb_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_bounce_count"), "set_reverb_bounce_count", "get_reverb_bounce_count");

    ClassDB::bind_method(D_METHOD("get_reverb_energy_cap"), &VAEmitter::get_reverb_energy_cap);
    ClassDB::bind_method(D_METHOD("set_reverb_energy_cap", "value"), &VAEmitter::set_reverb_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reverb_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_reverb_energy_cap", "get_reverb_energy_cap");

    ClassDB::bind_method(D_METHOD("get_max_volume"), &VAEmitter::get_max_volume);
    ClassDB::bind_method(D_METHOD("set_max_volume", "value"), &VAEmitter::set_max_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_volume", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_max_volume", "get_max_volume");

    ClassDB::bind_method(D_METHOD("get_max_echogram_time"), &VAEmitter::get_max_echogram_time);
    ClassDB::bind_method(D_METHOD("set_max_echogram_time", "value"), &VAEmitter::set_max_echogram_time);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_echogram_time"), "set_max_echogram_time", "get_max_echogram_time");

    ClassDB::bind_method(D_METHOD("get_echogram_granularity"), &VAEmitter::get_echogram_granularity);
    ClassDB::bind_method(D_METHOD("set_echogram_granularity", "value"), &VAEmitter::set_echogram_granularity);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "echogram_granularity"), "set_echogram_granularity", "get_echogram_granularity");

    ClassDB::bind_method(D_METHOD("get_affects_grouped_eax"), &VAEmitter::get_affects_grouped_eax);
    ClassDB::bind_method(D_METHOD("set_affects_grouped_eax", "value"), &VAEmitter::set_affects_grouped_eax);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "affects_grouped_eax"), "set_affects_grouped_eax", "get_affects_grouped_eax");

    ClassDB::bind_method(D_METHOD("get_has_relative_reverb"), &VAEmitter::get_has_relative_reverb);
    ClassDB::bind_method(D_METHOD("set_has_relative_reverb", "value"), &VAEmitter::set_has_relative_reverb);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "has_relative_reverb"), "set_has_relative_reverb", "get_has_relative_reverb");

    ClassDB::bind_method(D_METHOD("get_relative_reverb_inner_threshold"), &VAEmitter::get_relative_reverb_inner_threshold);
    ClassDB::bind_method(D_METHOD("set_relative_reverb_inner_threshold", "value"), &VAEmitter::set_relative_reverb_inner_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "relative_reverb_inner_threshold", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_relative_reverb_inner_threshold", "get_relative_reverb_inner_threshold");

    ClassDB::bind_method(D_METHOD("get_relative_reverb_outer_threshold"), &VAEmitter::get_relative_reverb_outer_threshold);
    ClassDB::bind_method(D_METHOD("set_relative_reverb_outer_threshold", "value"), &VAEmitter::set_relative_reverb_outer_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "relative_reverb_outer_threshold", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_relative_reverb_outer_threshold", "get_relative_reverb_outer_threshold");

    ADD_GROUP("Muffling", "");

    ClassDB::bind_method(D_METHOD("get_occlusion_ray_count"), &VAEmitter::get_occlusion_ray_count);
    ClassDB::bind_method(D_METHOD("set_occlusion_ray_count", "value"), &VAEmitter::set_occlusion_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "occlusion_ray_count"), "set_occlusion_ray_count", "get_occlusion_ray_count");

    ClassDB::bind_method(D_METHOD("get_occlusion_bounce_count"), &VAEmitter::get_occlusion_bounce_count);
    ClassDB::bind_method(D_METHOD("set_occlusion_bounce_count", "value"), &VAEmitter::set_occlusion_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "occlusion_bounce_count"), "set_occlusion_bounce_count", "get_occlusion_bounce_count");

    ClassDB::bind_method(D_METHOD("get_occlusion_energy_cap"), &VAEmitter::get_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_occlusion_energy_cap", "value"), &VAEmitter::set_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_occlusion_energy_cap", "get_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_permeation_ray_count"), &VAEmitter::get_permeation_ray_count);
    ClassDB::bind_method(D_METHOD("set_permeation_ray_count", "value"), &VAEmitter::set_permeation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "permeation_ray_count"), "set_permeation_ray_count", "get_permeation_ray_count");

    ClassDB::bind_method(D_METHOD("get_permeation_bounce_count"), &VAEmitter::get_permeation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_permeation_bounce_count", "value"), &VAEmitter::set_permeation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "permeation_bounce_count"), "set_permeation_bounce_count", "get_permeation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_permeation_energy_cap"), &VAEmitter::get_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_permeation_energy_cap", "value"), &VAEmitter::set_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_permeation_energy_cap", "get_permeation_energy_cap");

    ADD_GROUP("Ambience", "");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_ray_count"), &VAEmitter::get_ambient_occlusion_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_ray_count", "value"), &VAEmitter::set_ambient_occlusion_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_ray_count"), "set_ambient_occlusion_ray_count", "get_ambient_occlusion_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_bounce_count"), &VAEmitter::get_ambient_occlusion_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_bounce_count", "value"), &VAEmitter::set_ambient_occlusion_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_occlusion_bounce_count"), "set_ambient_occlusion_bounce_count", "get_ambient_occlusion_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_occlusion_energy_cap"), &VAEmitter::get_ambient_occlusion_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_occlusion_energy_cap", "value"), &VAEmitter::set_ambient_occlusion_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_occlusion_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_occlusion_energy_cap", "get_ambient_occlusion_energy_cap");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_ray_count"), &VAEmitter::get_ambient_permeation_ray_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_ray_count", "value"), &VAEmitter::set_ambient_permeation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_ray_count"), "set_ambient_permeation_ray_count", "get_ambient_permeation_ray_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_bounce_count"), &VAEmitter::get_ambient_permeation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_bounce_count", "value"), &VAEmitter::set_ambient_permeation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ambient_permeation_bounce_count"), "set_ambient_permeation_bounce_count", "get_ambient_permeation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_energy_cap"), &VAEmitter::get_ambient_permeation_energy_cap);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_energy_cap", "value"), &VAEmitter::set_ambient_permeation_energy_cap);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_permeation_energy_cap", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_ambient_permeation_energy_cap", "get_ambient_permeation_energy_cap");

    ADD_GROUP("Visualisation", "");

    ClassDB::bind_method(D_METHOD("get_visualisation_ray_count"), &VAEmitter::get_visualisation_ray_count);
    ClassDB::bind_method(D_METHOD("set_visualisation_ray_count", "value"), &VAEmitter::set_visualisation_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_ray_count"), "set_visualisation_ray_count", "get_visualisation_ray_count");

    ClassDB::bind_method(D_METHOD("get_visualisation_bounce_count"), &VAEmitter::get_visualisation_bounce_count);
    ClassDB::bind_method(D_METHOD("set_visualisation_bounce_count", "value"), &VAEmitter::set_visualisation_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_bounce_count"), "set_visualisation_bounce_count", "get_visualisation_bounce_count");

    ClassDB::bind_method(D_METHOD("get_visualisation_update_frequency"), &VAEmitter::get_visualisation_update_frequency);
    ClassDB::bind_method(D_METHOD("set_visualisation_update_frequency", "value"), &VAEmitter::set_visualisation_update_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visualisation_update_frequency"), "set_visualisation_update_frequency", "get_visualisation_update_frequency");

    ADD_GROUP("Advanced", "");

    ClassDB::bind_method(D_METHOD("get_type"), &VAEmitter::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "value"), &VAEmitter::set_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type"), "set_type", "get_type");

    ClassDB::bind_method(D_METHOD("get_refresh_ray_count"), &VAEmitter::get_refresh_ray_count);
    ClassDB::bind_method(D_METHOD("set_refresh_ray_count", "value"), &VAEmitter::set_refresh_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "refresh_ray_count"), "set_refresh_ray_count", "get_refresh_ray_count");

    ClassDB::bind_method(D_METHOD("get_refresh_distance_threshold"), &VAEmitter::get_refresh_distance_threshold);
    ClassDB::bind_method(D_METHOD("set_refresh_distance_threshold", "value"), &VAEmitter::set_refresh_distance_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_distance_threshold"), "set_refresh_distance_threshold", "get_refresh_distance_threshold");

    ClassDB::bind_method(D_METHOD("get_scattering_seed"), &VAEmitter::get_scattering_seed);
    ClassDB::bind_method(D_METHOD("set_scattering_seed", "value"), &VAEmitter::set_scattering_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scattering_seed"), "set_scattering_seed", "get_scattering_seed");

    ClassDB::bind_method(D_METHOD("get_clamp_position"), &VAEmitter::get_clamp_position);
    ClassDB::bind_method(D_METHOD("set_clamp_position", "value"), &VAEmitter::set_clamp_position);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clamp_position"), "set_clamp_position", "get_clamp_position");
}

VAEmitter::VAEmitter()
{
    // Matches VAEmitterProperties.cs's `Random.Shared.Next(int.MaxValue)`
    // field initializer - a random per-instance default so multiple emitters
    // don't produce identical scattering patterns unless explicitly set.
    scattering_seed = (int)(UtilityFunctions::randi() & 0x7fffffff);
}

VAEmitter::~VAEmitter()
{
    delete filter;
}

bool VAEmitter::get_is_main_listener() const
{
    return is_main_listener;
}

void VAEmitter::set_is_main_listener(bool value)
{
    is_main_listener = value;
}

void VAEmitter::_enter_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    va_world = find_va_world(this);

    if (!va_world)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] VAEmitter has no sibling VAWorld node. Node: ", get_name());
        return;
    }

    create_emitter();
}

void VAEmitter::_exit_tree()
{
    if (emitter)
    {
        remove_emitter();
    }
}

void VAEmitter::create_emitter()
{
    emitter = vaEmitterCreate();
    vaEmitterSetUserData(emitter, this);
    vaEmitterSetName(emitter, String(get_name()).utf8().get_data());
    vaEmitterSetPosition(emitter, ToVAudio(get_global_position()));

    vaEmitterSetOnRaytracingCompleteCallback(emitter, &VAEmitter::on_raytracing_complete_trampoline);
    vaEmitterSetOnRaytracedByAnotherEmitterCallback(emitter, &VAEmitter::on_raytraced_by_another_emitter_trampoline);
    vaEmitterSetOnRemovedCallback(emitter, &VAEmitter::on_removed_trampoline);

    // Matches VAWorld.cs's CreateEmitter: the listener additionally gets
    // HasRelativeReverb=true (used for relative-direction/relative-gain
    // reverb blending, a grouped-EAX-only concern - harmless to set now,
    // becomes meaningful once grouped-EAX is implemented).
    if (is_main_listener)
    {
        vaEmitterSetHasRelativeReverb(emitter, true);

        // Without either of these, the listener never casts rays toward
        // VASource targets, so has_raytraced_target/get_target_filter
        // (va_source.cpp's apply_raytracing_results) never fires and
        // sources are never muffled - easy to miss since nothing else
        // fails, sound just plays unmuffled through walls.
        if (occlusion_ray_count == 0 && permeation_ray_count == 0)
        {
            UtilityFunctions::push_warning(
                "[vaudio-godot-native-openal] Main listener '", get_name(),
                "' has occlusion_ray_count and permeation_ray_count both set to 0 - "
                "sources will never be muffled. Set one or both to enable muffling.");
        }
    }

    apply_properties_to_handle();

    va_world->register_emitter(this, is_main_listener);
}

void VAEmitter::apply_properties_to_handle()
{
    vaEmitterSetReverbRayCount(emitter, reverb_ray_count);
    vaEmitterSetReverbBounceCount(emitter, reverb_bounce_count);
    vaEmitterSetReverbEnergyCap(emitter, reverb_energy_cap);
    vaEmitterSetMaxVolume(emitter, max_volume);
    vaEmitterSetMaxEchogramTime(emitter, max_echogram_time);
    vaEmitterSetEchogramGranularity(emitter, echogram_granularity);
    vaEmitterSetAffectsGroupedEAX(emitter, affects_grouped_eax);
    vaEmitterSetHasRelativeReverb(emitter, has_relative_reverb);
    vaEmitterSetRelativeReverbInnerThreshold(emitter, relative_reverb_inner_threshold);
    vaEmitterSetRelativeReverbOuterThreshold(emitter, relative_reverb_outer_threshold);

    vaEmitterSetOcclusionRayCount(emitter, occlusion_ray_count);
    vaEmitterSetOcclusionBounceCount(emitter, occlusion_bounce_count);
    vaEmitterSetOcclusionEnergyCap(emitter, occlusion_energy_cap);
    vaEmitterSetPermeationRayCount(emitter, permeation_ray_count);
    vaEmitterSetPermeationBounceCount(emitter, permeation_bounce_count);
    vaEmitterSetPermeationEnergyCap(emitter, permeation_energy_cap);

    vaEmitterSetAmbientOcclusionRayCount(emitter, ambient_occlusion_ray_count);
    vaEmitterSetAmbientOcclusionBounceCount(emitter, ambient_occlusion_bounce_count);
    vaEmitterSetAmbientOcclusionEnergyCap(emitter, ambient_occlusion_energy_cap);
    vaEmitterSetAmbientPermeationRayCount(emitter, ambient_permeation_ray_count);
    vaEmitterSetAmbientPermeationBounceCount(emitter, ambient_permeation_bounce_count);
    vaEmitterSetAmbientPermeationEnergyCap(emitter, ambient_permeation_energy_cap);

    vaEmitterSetVisualisationRayCount(emitter, visualisation_ray_count);
    vaEmitterSetVisualisationBounceCount(emitter, visualisation_bounce_count);
    vaEmitterSetVisualisationUpdateFrequency(emitter, visualisation_update_frequency);

    vaEmitterSetType(emitter, type);
    vaEmitterSetRefreshRayCount(emitter, refresh_ray_count);
    vaEmitterSetRefreshDistanceThreshold(emitter, refresh_distance_threshold);
    vaEmitterSetScatteringSeed(emitter, scattering_seed);
    vaEmitterSetClampPosition(emitter, clamp_position);
}

void VAEmitter::remove_emitter()
{
    // vaWorldRemoveEmitter detaches the emitter from the world and, once any
    // pending reverb tail finishes, invokes the OnRemoved callback
    // (on_removed_trampoline -> on_emitter_removed, which hands the handle to
    // VAWorld for deferred destruction - see on_emitter_removed's doc
    // comment). Matches VAEmitter.cs's RemoveEmitter, which deliberately
    // doesn't null `emitter` here either, for the same reason - the SDK
    // doesn't destroy the handle synchronously.
    vaWorldRemoveEmitter(va_world->get_handle(), emitter);
}

bool VAEmitter::is_raytraced() const
{
    return emitter && !vaEmitterGetInitialising(emitter);
}

Vector3 VAEmitter::get_va_position() const
{
    VAVector position = vaEmitterGetPosition(emitter);
    return Vector3(position.x, position.y, position.z);
}

bool VAEmitter::get_within_world_bounds() const
{
    return vaEmitterGetWithinWorldBounds(emitter);
}

void VAEmitter::add_target(VAEmitter *target)
{
    vaEmitterAddTarget(emitter, target->get_handle());
}

bool VAEmitter::has_raytraced_target(VAEmitter *target) const
{
    return vaEmitterHasRaytracedTarget(emitter, target->get_handle());
}

VALowPassFilter VAEmitter::get_target_filter(VAEmitter *target) const
{
    return *vaEmitterGetTargetFilter(emitter, target->get_handle());
}

void VAEmitter::_process(double delta)
{
    if (!emitter)
    {
        return;
    }

    vaEmitterSetPosition(emitter, ToVAudio(get_global_position()));

    if (is_raytraced())
    {
        apply_raytracing_results();
    }
}

// VAEmitter.cs's ApplyRaytracingResults port: resolves which reverb slot this
// emitter uses (unconditionally, even for the listener itself), then - only
// for non-listener emitters - updates the muffling filter from the
// listener's perspective (how muffled this emitter sounds to the listener).
void VAEmitter::apply_raytracing_results()
{
    effect = va_world->get_reverb_effect(emitter);

    VAEmitter *listener = va_world->get_listener();
    if (!listener)
    {
        return;
    }

    if (this != listener && listener->has_raytraced_target(this))
    {
        VALowPassFilter vaudio_filter = listener->get_target_filter(this);

        if (filter)
        {
            filter->set_gain(vaudio_filter.gainLF, vaudio_filter.gainHF);
        }
    }
}

void VAEmitter::on_raytraced_by_another_emitter(::VAEmitter *other)
{
    if (!filter)
    {
        filter = new ALFilter();
        filter->create(1.0f, 1.0f);
    }

    apply_raytracing_results();
}

void VAEmitter::on_emitter_removed()
{
    // Deliberately not vaEmitterDestroy(emitter) here - see the warning on
    // this method's declaration in va_emitter.h. VAWorld owns final
    // destruction, deferred until after vaWorldWait() has fully drained.
    va_world->defer_emitter_destroy(emitter);
    emitter = nullptr;
}

void VAEmitter::on_raytracing_complete_trampoline(::VAEmitter *emitter)
{
    // No VAEmitter-level behaviour needed yet - matches VAEmitter.cs's
    // OnRaytracingComplete, which just forwards to an optional external
    // callback nothing in this native port currently subscribes to.
}

void VAEmitter::on_raytraced_by_another_emitter_trampoline(::VAEmitter *source, ::VAEmitter *target)
{
    // Registered on (and invoked as a method of) target - the emitter being
    // raytraced, i.e. "this" - matching world_emitters.c's
    // target->onRaytracedByAnotherEmitter(source, target). source is who did
    // the raytracing (VAEmitter.cs's OnRaytracedByAnotherEmitter(vaudio.Emitter
    // emitter) parameter - only used to know the event fired, not stored).
    VAEmitter *self = static_cast<VAEmitter *>(vaEmitterGetUserData(target));

    if (self)
    {
        self->on_raytraced_by_another_emitter(source);
    }
}

void VAEmitter::on_removed_trampoline(::VAEmitter *emitter)
{
    VAEmitter *self = static_cast<VAEmitter *>(vaEmitterGetUserData(emitter));

    if (self)
    {
        self->on_emitter_removed();
    }
}

// Exported tuning-knob getter/setter bodies - direct port of
// VAEmitterProperties.cs, including its Math.Max(0, value) clamps on
// ray/bounce counts and energy caps.

int VAEmitter::get_reverb_ray_count() const
{
    return reverb_ray_count;
}

void VAEmitter::set_reverb_ray_count(int value)
{
    reverb_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetReverbRayCount(emitter, reverb_ray_count);
    }
}

int VAEmitter::get_reverb_bounce_count() const
{
    return reverb_bounce_count;
}

void VAEmitter::set_reverb_bounce_count(int value)
{
    reverb_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetReverbBounceCount(emitter, reverb_bounce_count);
    }
}

float VAEmitter::get_reverb_energy_cap() const
{
    return reverb_energy_cap;
}

void VAEmitter::set_reverb_energy_cap(float value)
{
    reverb_energy_cap = value;

    if (emitter)
    {
        vaEmitterSetReverbEnergyCap(emitter, reverb_energy_cap);
    }
}

float VAEmitter::get_max_volume() const
{
    return max_volume;
}

void VAEmitter::set_max_volume(float value)
{
    max_volume = value;

    if (emitter)
    {
        vaEmitterSetMaxVolume(emitter, max_volume);
    }
}

int VAEmitter::get_max_echogram_time() const
{
    return max_echogram_time;
}

void VAEmitter::set_max_echogram_time(int value)
{
    max_echogram_time = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetMaxEchogramTime(emitter, max_echogram_time);
    }
}

int VAEmitter::get_echogram_granularity() const
{
    return echogram_granularity;
}

void VAEmitter::set_echogram_granularity(int value)
{
    echogram_granularity = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetEchogramGranularity(emitter, echogram_granularity);
    }
}

bool VAEmitter::get_affects_grouped_eax() const
{
    return affects_grouped_eax;
}

void VAEmitter::set_affects_grouped_eax(bool value)
{
    affects_grouped_eax = value;

    if (emitter)
    {
        vaEmitterSetAffectsGroupedEAX(emitter, affects_grouped_eax);
    }
}

bool VAEmitter::get_has_relative_reverb() const
{
    return has_relative_reverb;
}

void VAEmitter::set_has_relative_reverb(bool value)
{
    has_relative_reverb = value;

    if (emitter)
    {
        vaEmitterSetHasRelativeReverb(emitter, has_relative_reverb);
    }
}

float VAEmitter::get_relative_reverb_inner_threshold() const
{
    return relative_reverb_inner_threshold;
}

void VAEmitter::set_relative_reverb_inner_threshold(float value)
{
    relative_reverb_inner_threshold = value;

    if (emitter)
    {
        vaEmitterSetRelativeReverbInnerThreshold(emitter, relative_reverb_inner_threshold);
    }
}

float VAEmitter::get_relative_reverb_outer_threshold() const
{
    return relative_reverb_outer_threshold;
}

void VAEmitter::set_relative_reverb_outer_threshold(float value)
{
    relative_reverb_outer_threshold = value;

    if (emitter)
    {
        vaEmitterSetRelativeReverbOuterThreshold(emitter, relative_reverb_outer_threshold);
    }
}

int VAEmitter::get_occlusion_ray_count() const
{
    return occlusion_ray_count;
}

void VAEmitter::set_occlusion_ray_count(int value)
{
    occlusion_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetOcclusionRayCount(emitter, occlusion_ray_count);
    }
}

int VAEmitter::get_occlusion_bounce_count() const
{
    return occlusion_bounce_count;
}

void VAEmitter::set_occlusion_bounce_count(int value)
{
    occlusion_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetOcclusionBounceCount(emitter, occlusion_bounce_count);
    }
}

float VAEmitter::get_occlusion_energy_cap() const
{
    return occlusion_energy_cap;
}

void VAEmitter::set_occlusion_energy_cap(float value)
{
    occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        vaEmitterSetOcclusionEnergyCap(emitter, occlusion_energy_cap);
    }
}

int VAEmitter::get_permeation_ray_count() const
{
    return permeation_ray_count;
}

void VAEmitter::set_permeation_ray_count(int value)
{
    permeation_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetPermeationRayCount(emitter, permeation_ray_count);
    }
}

int VAEmitter::get_permeation_bounce_count() const
{
    return permeation_bounce_count;
}

void VAEmitter::set_permeation_bounce_count(int value)
{
    permeation_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetPermeationBounceCount(emitter, permeation_bounce_count);
    }
}

float VAEmitter::get_permeation_energy_cap() const
{
    return permeation_energy_cap;
}

void VAEmitter::set_permeation_energy_cap(float value)
{
    permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        vaEmitterSetPermeationEnergyCap(emitter, permeation_energy_cap);
    }
}

int VAEmitter::get_ambient_occlusion_ray_count() const
{
    return ambient_occlusion_ray_count;
}

void VAEmitter::set_ambient_occlusion_ray_count(int value)
{
    ambient_occlusion_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetAmbientOcclusionRayCount(emitter, ambient_occlusion_ray_count);
    }
}

int VAEmitter::get_ambient_occlusion_bounce_count() const
{
    return ambient_occlusion_bounce_count;
}

void VAEmitter::set_ambient_occlusion_bounce_count(int value)
{
    ambient_occlusion_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetAmbientOcclusionBounceCount(emitter, ambient_occlusion_bounce_count);
    }
}

float VAEmitter::get_ambient_occlusion_energy_cap() const
{
    return ambient_occlusion_energy_cap;
}

void VAEmitter::set_ambient_occlusion_energy_cap(float value)
{
    ambient_occlusion_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        vaEmitterSetAmbientOcclusionEnergyCap(emitter, ambient_occlusion_energy_cap);
    }
}

int VAEmitter::get_ambient_permeation_ray_count() const
{
    return ambient_permeation_ray_count;
}

void VAEmitter::set_ambient_permeation_ray_count(int value)
{
    ambient_permeation_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetAmbientPermeationRayCount(emitter, ambient_permeation_ray_count);
    }
}

int VAEmitter::get_ambient_permeation_bounce_count() const
{
    return ambient_permeation_bounce_count;
}

void VAEmitter::set_ambient_permeation_bounce_count(int value)
{
    ambient_permeation_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetAmbientPermeationBounceCount(emitter, ambient_permeation_bounce_count);
    }
}

float VAEmitter::get_ambient_permeation_energy_cap() const
{
    return ambient_permeation_energy_cap;
}

void VAEmitter::set_ambient_permeation_energy_cap(float value)
{
    ambient_permeation_energy_cap = MAX(0.0f, value);

    if (emitter)
    {
        vaEmitterSetAmbientPermeationEnergyCap(emitter, ambient_permeation_energy_cap);
    }
}

int VAEmitter::get_visualisation_ray_count() const
{
    return visualisation_ray_count;
}

void VAEmitter::set_visualisation_ray_count(int value)
{
    visualisation_ray_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetVisualisationRayCount(emitter, visualisation_ray_count);
    }
}

int VAEmitter::get_visualisation_bounce_count() const
{
    return visualisation_bounce_count;
}

void VAEmitter::set_visualisation_bounce_count(int value)
{
    visualisation_bounce_count = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetVisualisationBounceCount(emitter, visualisation_bounce_count);
    }
}

int VAEmitter::get_visualisation_update_frequency() const
{
    return visualisation_update_frequency;
}

void VAEmitter::set_visualisation_update_frequency(int value)
{
    visualisation_update_frequency = MAX(0, value);

    if (emitter)
    {
        vaEmitterSetVisualisationUpdateFrequency(emitter, visualisation_update_frequency);
    }
}

int VAEmitter::get_type() const
{
    return type;
}

void VAEmitter::set_type(int value)
{
    type = value;

    if (emitter)
    {
        vaEmitterSetType(emitter, type);
    }
}

int VAEmitter::get_refresh_ray_count() const
{
    return refresh_ray_count;
}

void VAEmitter::set_refresh_ray_count(int value)
{
    refresh_ray_count = value;

    if (emitter)
    {
        vaEmitterSetRefreshRayCount(emitter, refresh_ray_count);
    }
}

float VAEmitter::get_refresh_distance_threshold() const
{
    return refresh_distance_threshold;
}

void VAEmitter::set_refresh_distance_threshold(float value)
{
    refresh_distance_threshold = value;

    if (emitter)
    {
        vaEmitterSetRefreshDistanceThreshold(emitter, refresh_distance_threshold);
    }
}

int VAEmitter::get_scattering_seed() const
{
    return scattering_seed;
}

void VAEmitter::set_scattering_seed(int value)
{
    scattering_seed = value;

    if (emitter)
    {
        vaEmitterSetScatteringSeed(emitter, scattering_seed);
    }
}

bool VAEmitter::get_clamp_position() const
{
    return clamp_position;
}

void VAEmitter::set_clamp_position(bool value)
{
    clamp_position = value;

    if (emitter)
    {
        vaEmitterSetClampPosition(emitter, clamp_position);
    }
}

} // namespace va_godot
