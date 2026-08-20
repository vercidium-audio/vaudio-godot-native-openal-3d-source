#include "va_emitter.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_filter.h"
#include "openal/al_reverb.h"
#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

namespace va_godot
{

void VAEmitter::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_is_main_listener"), &VAEmitter::get_is_main_listener);
    ClassDB::bind_method(D_METHOD("set_is_main_listener", "value"), &VAEmitter::set_is_main_listener);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_main_listener"), "set_is_main_listener", "get_is_main_listener");

    ClassDB::bind_method(D_METHOD("get_raytrace_once"), &VAEmitter::get_raytrace_once);
    ClassDB::bind_method(D_METHOD("set_raytrace_once", "value"), &VAEmitter::set_raytrace_once);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "raytrace_once"), "set_raytrace_once", "get_raytrace_once");

    // Read-only SDK forwards and ambient filter stats (listener only) - not ADD_PROPERTY'd, called directly from GDScript like methods.
    ClassDB::bind_method(D_METHOD("get_va_position"), &VAEmitter::get_va_position);
    ClassDB::bind_method(D_METHOD("get_within_world_bounds"), &VAEmitter::get_within_world_bounds);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VAEmitter::is_raytraced);
    ClassDB::bind_method(D_METHOD("get_eax_debug_info"), &VAEmitter::get_eax_debug_info);

    ClassDB::bind_method(D_METHOD("is_ambient_filter_ready"), &VAEmitter::is_ambient_filter_ready);
    ClassDB::bind_method(D_METHOD("get_ambient_filter_gain_lf"), &VAEmitter::get_ambient_filter_gain_lf);
    ClassDB::bind_method(D_METHOD("get_ambient_filter_gain_hf"), &VAEmitter::get_ambient_filter_gain_hf);

    // Direct port of VAEmitterProperties.cs groups (Reverb/Muffling/Ambience/Visualisation/Advanced).
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

    ClassDB::bind_method(D_METHOD("get_use_listener_reverb"), &VAEmitter::get_use_listener_reverb);
    ClassDB::bind_method(D_METHOD("set_use_listener_reverb", "value"), &VAEmitter::set_use_listener_reverb);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_listener_reverb"), "set_use_listener_reverb", "get_use_listener_reverb");

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

    ADD_GROUP("Debug Rendering", "");

    ClassDB::bind_method(D_METHOD("get_random_trail_color"), &VAEmitter::get_random_trail_color);
    ClassDB::bind_method(D_METHOD("set_random_trail_color", "value"), &VAEmitter::set_random_trail_color);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_trail_color"), "set_random_trail_color", "get_random_trail_color");

    ClassDB::bind_method(D_METHOD("get_trail_color"), &VAEmitter::get_trail_color);
    ClassDB::bind_method(D_METHOD("set_trail_color", "value"), &VAEmitter::set_trail_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "trail_color"), "set_trail_color", "get_trail_color");

    ClassDB::bind_method(D_METHOD("get_reverb_color"), &VAEmitter::get_reverb_color);
    ClassDB::bind_method(D_METHOD("set_reverb_color", "value"), &VAEmitter::set_reverb_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "reverb_color"), "set_reverb_color", "get_reverb_color");

    ClassDB::bind_method(D_METHOD("get_occlusion_color"), &VAEmitter::get_occlusion_color);
    ClassDB::bind_method(D_METHOD("set_occlusion_color", "value"), &VAEmitter::set_occlusion_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "occlusion_color"), "set_occlusion_color", "get_occlusion_color");

    ClassDB::bind_method(D_METHOD("get_permeation_color"), &VAEmitter::get_permeation_color);
    ClassDB::bind_method(D_METHOD("set_permeation_color", "value"), &VAEmitter::set_permeation_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "permeation_color"), "set_permeation_color", "get_permeation_color");

    ClassDB::bind_method(D_METHOD("get_ambient_permeation_color"), &VAEmitter::get_ambient_permeation_color);
    ClassDB::bind_method(D_METHOD("set_ambient_permeation_color", "value"), &VAEmitter::set_ambient_permeation_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ambient_permeation_color"), "set_ambient_permeation_color", "get_ambient_permeation_color");
}

VAEmitter::VAEmitter()
{
    scattering_seed = (int)(UtilityFunctions::randi() & 0x7fffffff); // Random scattering seed for every emitter
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

bool VAEmitter::get_raytrace_once() const
{
    return raytrace_once;
}

void VAEmitter::set_raytrace_once(bool value)
{
    raytrace_once = value;
}

// Hides is_main_listener from the inspector - VAListener is the intended way to designate a world's listener (see va_listener.cpp).
void VAEmitter::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("is_main_listener"))
        p_property.usage = PROPERTY_USAGE_NONE;
}

void VAEmitter::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    va_world = find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on every future node addition instead of erroring out permanently, see retry_find_va_world.
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VAEmitter::retry_find_va_world));
        return;
    }

    create_emitter();
}

void VAEmitter::_exit_tree()
{
    if (waiting_for_world)
    {
        // Disconnect the "node_added" callback
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VAEmitter::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VAEmitter::retry_find_va_world));
        }

        waiting_for_world = false;

        VA_WARN(
            "'", get_name(),
            "' left the tree without ever finding a VAWorld - no emitter was created for it. Make sure this node's scene was added under a VAWorld while it was in the tree.");
    }

    if (emitter)
    {
        va_world->unregister_pending_target(this); // No-op unless this emitter was still waiting in pending_targets for a listener to appear
        va_world->unregister_listener(this); // No-op unless this is va_world's current listener - avoids a dangling pointer if freed before VAWorld

        remove_emitter();
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree; once a VAWorld becomes reachable, disconnects and initialises normally via create_emitter().
void VAEmitter::retry_find_va_world(Node *node)
{
    va_world = find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VAEmitter::retry_find_va_world));
    waiting_for_world = false;

    create_emitter();
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

    // Matches VAWorld.cs's CreateEmitter: the listener additionally gets HasRelativeReverb=true, a grouped-EAX-only concern harmless to set now.
    if (is_main_listener)
    {
        vaEmitterSetHasRelativeReverb(emitter, true);

        // Without either of these, the listener never casts rays toward VASource targets, so sources are never muffled - easy to miss since nothing else fails.
        if (occlusion_ray_count == 0 && permeation_ray_count == 0)
        {
            VA_WARN(
                "Main listener '", get_name(),
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

    vaEmitterSetType(emitter, type);
    vaEmitterSetRefreshRayCount(emitter, refresh_ray_count);
    vaEmitterSetRefreshDistanceThreshold(emitter, refresh_distance_threshold);
    vaEmitterSetScatteringSeed(emitter, scattering_seed);
    vaEmitterSetClampPosition(emitter, clamp_position);

    vaEmitterSetRandomTrailColor(emitter, random_trail_color);
    vaEmitterSetTrailColor(emitter, ToVAudio(trail_color));
    vaEmitterSetReverbColor(emitter, ToVAudio(reverb_color));
    vaEmitterSetOcclusionColor(emitter, ToVAudio(occlusion_color));
    vaEmitterSetPermeationColor(emitter, ToVAudio(permeation_color));
    vaEmitterSetAmbientPermeationColor(emitter, ToVAudio(ambient_permeation_color));
}

void VAEmitter::remove_emitter()
{
    // vaWorldRemoveEmitter detaches the emitter and, once any pending reverb tail finishes, invokes OnRemoved (-> on_emitter_removed,
    // which defers destruction to VAWorld). Doesn't null `emitter` here either, matching VAEmitter.cs, since the SDK doesn't destroy synchronously.
    VAResult result = vaWorldRemoveEmitter(va_world->get_handle(), emitter);

    // VA_PENDING_REMOVAL just means the reverb tail hasn't finished yet - OnRemoved will still fire once it has. Not an error.
    if (result != VA_SUCCESS && result != VA_PENDING_REMOVAL)
    {
        VA_ERROR(
            "VAEmitter::remove_emitter failed for '", get_name(),
            "' (VAResult=", VAResultToString(result), ")");
    }
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

Dictionary VAEmitter::get_eax_debug_info() const
{
    Dictionary info;

    if (!emitter)
        return info;

    VAEAXReverb *eax = vaEmitterGetEAX(emitter);

    if (!eax)
        return info;

    info["outside_percent"] = eax->outsidePercent;
    info["returned_percent"] = eax->returnedPercent;
    info["material_absorption_lf"] = eax->materialAbsorptionLF;
    info["material_absorption_hf"] = eax->materialAbsorptionHF;
    info["material_roughness"] = eax->materialRoughness;
    info["reflections_delay"] = eax->reflectionsDelay;
    info["density"] = eax->density;
    info["diffusion"] = eax->diffusion;
    info["gain_lf"] = eax->gainLF;
    info["gain_hf"] = eax->gainHF;
    info["gain"] = eax->gain;
    info["decay_time"] = eax->decayTime;
    info["decay_lf_ratio"] = eax->decayLFRatio;
    info["decay_hf_ratio"] = eax->decayHFRatio;
    info["reflections_gain"] = eax->reflectionsGain;
    info["late_reverb_gain"] = eax->lateReverbGain;
    info["late_reverb_delay"] = eax->lateReverbDelay;
    info["echo_time"] = eax->echoTime;
    info["echo_depth"] = eax->echoDepth;
    info["modulation_time"] = eax->modulationTime;
    info["modulation_depth"] = eax->modulationDepth;
    info["air_absorption_gain_hf"] = eax->airAbsorptionGainHF;
    info["hf_reference"] = eax->hfReference;
    info["lf_reference"] = eax->lfReference;
    info["room_rolloff_factor"] = eax->roomRolloffFactor;
    info["decay_hf_limit"] = (bool)eax->decayHFLimit;

    return info;
}

void VAEmitter::add_target(VAEmitter *target)
{
    VAResult result = vaEmitterAddTarget(emitter, target->get_handle());

    switch (result)
    {
        case VA_SUCCESS:
        case VA_ALREADY_EXISTS:
            // Matches VAEmitter.cs's AddTarget, which treats re-adding an existing target as a no-op rather than an error.
            break;

        case VA_NOT_ADDED_TO_WORLD:
            VA_ERROR(
                "VAEmitter::add_target failed for '", get_name(),
                "' -> '", target->get_name(), "': target has not been added to the same VAWorld as this emitter.");
            break;

        case VA_FEATURE_DISABLED:
            VA_ERROR(
                "VAEmitter::add_target failed for '", get_name(),
                "' -> '", target->get_name(), "': this emitter casts neither occlusion nor permeation rays, "
                "so it cannot have targets. Set occlusion_ray_count or permeation_ray_count above 0 first.");
            break;

        default:
            VA_ERROR(
                "VAEmitter::add_target failed for '", get_name(),
                "' -> '", target->get_name(), "' (VAResult=", VAResultToString(result), ")");
            break;
    }
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

// VAEmitter.cs's ApplyRaytracingResults port: resolves which reverb slot this emitter uses (even for the listener itself), then for
// non-listener emitters only, updates the muffling filter from the listener's perspective (how muffled this emitter sounds to the listener).
void VAEmitter::apply_raytracing_results()
{
    effect = va_world->get_reverb_effect(emitter);

    VAEmitter *listener = va_world->get_listener();
    if (!listener)
    {
        return;
    }

    if (this == listener)
    {
        // VAWorldReverb.cs's OnReverbUpdated ambientFilter update, moved onto the listener it actually describes - only its own ambient filter is meaningful.
        VALowPassFilter *ambient_filter = vaEmitterGetAmbientFilter(emitter);

        if (ambient_filter)
        {
            ambient_filter_gain_lf = ambient_filter->gainLF;
            ambient_filter_gain_hf = ambient_filter->gainHF;
            ambient_filter_ready = true;
        }
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

    // Matches VAEmitter.cs's OnRaytracedByAnotherEmitter: once raytraced once, cast rays no more - remove it from the world.
    if (raytrace_once)
    {
        remove_emitter();
    }
}

void VAEmitter::on_emitter_removed()
{
    // Deliberately not vaEmitterDestroy(emitter) here - see the warning on this method's declaration in va_emitter.h. VAWorld owns final destruction, deferred until vaWorldWait() has fully drained.
    va_world->defer_emitter_destroy(emitter);
    emitter = nullptr;
}

void VAEmitter::on_raytracing_complete_trampoline(::VAEmitter *emitter)
{
    // No VAEmitter-level behaviour needed yet - nothing in this native port currently subscribes to this callback.
}

void VAEmitter::on_raytraced_by_another_emitter_trampoline(::VAEmitter *source, ::VAEmitter *target)
{
    // Registered on (and invoked as a method of) target - the emitter being raytraced, i.e. "this", matching world_emitters.c's target->onRaytracedByAnotherEmitter(source, target).
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

// Exported tuning-knob getter/setter bodies - direct port of VAEmitterProperties.cs, including its Math.Max(0, value) clamps.

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

bool VAEmitter::get_use_listener_reverb() const
{
    return use_listener_reverb;
}

void VAEmitter::set_use_listener_reverb(bool value)
{
    use_listener_reverb = value;
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
        vaEmitterSetAmbientPermeationRayCount(emitter, ambient_permeation_ray_count);
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

bool VAEmitter::get_random_trail_color() const
{
    return random_trail_color;
}

void VAEmitter::set_random_trail_color(bool value)
{
    random_trail_color = value;

    if (emitter)
    {
        vaEmitterSetRandomTrailColor(emitter, random_trail_color);
    }
}

Color VAEmitter::get_trail_color() const
{
    return trail_color;
}

void VAEmitter::set_trail_color(const Color &value)
{
    trail_color = value;

    if (emitter)
    {
        vaEmitterSetTrailColor(emitter, ToVAudio(trail_color));
    }
}

Color VAEmitter::get_reverb_color() const
{
    return reverb_color;
}

void VAEmitter::set_reverb_color(const Color &value)
{
    reverb_color = value;

    if (emitter)
    {
        vaEmitterSetReverbColor(emitter, ToVAudio(reverb_color));
    }
}

Color VAEmitter::get_occlusion_color() const
{
    return occlusion_color;
}

void VAEmitter::set_occlusion_color(const Color &value)
{
    occlusion_color = value;

    if (emitter)
    {
        vaEmitterSetOcclusionColor(emitter, ToVAudio(occlusion_color));
    }
}

Color VAEmitter::get_permeation_color() const
{
    return permeation_color;
}

void VAEmitter::set_permeation_color(const Color &value)
{
    permeation_color = value;

    if (emitter)
    {
        vaEmitterSetPermeationColor(emitter, ToVAudio(permeation_color));
    }
}

Color VAEmitter::get_ambient_permeation_color() const
{
    return ambient_permeation_color;
}

void VAEmitter::set_ambient_permeation_color(const Color &value)
{
    ambient_permeation_color = value;

    if (emitter)
    {
        vaEmitterSetAmbientPermeationColor(emitter, ToVAudio(ambient_permeation_color));
    }
}

} // namespace va_godot
