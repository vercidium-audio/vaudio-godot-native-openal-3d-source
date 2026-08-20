#include "va_world.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_manager.h"
#include "va_conversions.h"
#include "va_emitter.h"
#include "va_custom_material.h"
#include "va_engine_util.h"

#include <algorithm>

namespace va_godot
{

// Smallest id reserved for custom (non-built-in) materials - matches vaudio.h's VAMaterialType comment and vaudio-unreal's FirstCustomMaterialId.
static constexpr int FirstCustomMaterialId = 1000;

void VAWorld::_bind_methods()
{
    // Rebinds the inherited Node3D "position" property to VAWorld's own accessors, so moving this node also updates vaWorldSetPosition - see va_world_properties.cpp.
    ClassDB::bind_method(D_METHOD("get_position"), &VAWorld::get_position);
    ClassDB::bind_method(D_METHOD("set_position", "value"), &VAWorld::set_position);
    ClassDB::bind_method(D_METHOD("get_bounds_size"), &VAWorld::get_bounds_size);
    ClassDB::bind_method(D_METHOD("set_bounds_size", "value"), &VAWorld::set_bounds_size);
    ClassDB::bind_method(D_METHOD("get_bounds_color"), &VAWorld::get_bounds_color);
    ClassDB::bind_method(D_METHOD("set_bounds_color", "value"), &VAWorld::set_bounds_color);
    ClassDB::bind_method(D_METHOD("get_epsilon"), &VAWorld::get_epsilon);
    ClassDB::bind_method(D_METHOD("set_epsilon", "value"), &VAWorld::set_epsilon);
    ClassDB::bind_method(D_METHOD("get_world_is_indoors"), &VAWorld::get_world_is_indoors);
    ClassDB::bind_method(D_METHOD("set_world_is_indoors", "value"), &VAWorld::set_world_is_indoors);

    ADD_GROUP("World", "");

    // Without this, ClassDB still resolves the inherited "position" property to Node3D's own accessors, so set_position (bound above) would never be called through the property system.
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position", PROPERTY_HINT_RANGE, "-1000,1000,1,or_less,or_greater"), "set_position", "get_position");

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "bounds_size", PROPERTY_HINT_RANGE, "1,1000,1,or_greater"), "set_bounds_size", "get_bounds_size");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "bounds_color"), "set_bounds_color", "get_bounds_color");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "epsilon"), "set_epsilon", "get_epsilon");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "world_is_indoors"), "set_world_is_indoors", "get_world_is_indoors");

    ClassDB::bind_method(D_METHOD("get_master_volume"), &VAWorld::get_master_volume);
    ClassDB::bind_method(D_METHOD("set_master_volume", "value"), &VAWorld::set_master_volume);
    ClassDB::bind_method(D_METHOD("get_distance_model"), &VAWorld::get_distance_model);
    ClassDB::bind_method(D_METHOD("set_distance_model", "value"), &VAWorld::set_distance_model);
    ClassDB::bind_method(D_METHOD("get_reverb_only"), &VAWorld::get_reverb_only);
    ClassDB::bind_method(D_METHOD("set_reverb_only", "value"), &VAWorld::set_reverb_only);

    // OpenAL settings, forwarded to the ALManager singleton - if multiple VAWorlds exist in a scene, the last write wins.
    ADD_GROUP("OpenAL", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "master_volume"), "set_master_volume", "get_master_volume");
    // Matches vaudio-godot-mono-openal-3d's ALDistanceModel enum order/values (AL/al.h).
    ADD_PROPERTY(PropertyInfo(Variant::INT, "distance_model", PROPERTY_HINT_ENUM, "None:0,InverseDistance:53249,InverseDistanceClamped:53250,LinearDistance:53251,LinearDistanceClamped:53252,ExponentDistance:53253,ExponentDistanceClamped:53254"), "set_distance_model", "get_distance_model");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverb_only"), "set_reverb_only", "get_reverb_only");

    ClassDB::bind_method(D_METHOD("get_maximum_grouped_eax_count"), &VAWorld::get_maximum_grouped_eax_count);
    ClassDB::bind_method(D_METHOD("set_maximum_grouped_eax_count", "value"), &VAWorld::set_maximum_grouped_eax_count);

    ADD_GROUP("Reverb", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_grouped_eax_count", PROPERTY_HINT_RANGE, "1,32,1,or_greater"), "set_maximum_grouped_eax_count", "get_maximum_grouped_eax_count");

    ClassDB::bind_method(D_METHOD("get_meters_per_unit"), &VAWorld::get_meters_per_unit);
    ClassDB::bind_method(D_METHOD("set_meters_per_unit", "value"), &VAWorld::set_meters_per_unit);
    ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &VAWorld::get_speed_of_sound);
    ClassDB::bind_method(D_METHOD("set_speed_of_sound", "value"), &VAWorld::set_speed_of_sound);
    ClassDB::bind_method(D_METHOD("get_humidity"), &VAWorld::get_humidity);
    ClassDB::bind_method(D_METHOD("set_humidity", "value"), &VAWorld::set_humidity);
    ClassDB::bind_method(D_METHOD("get_temperature"), &VAWorld::get_temperature);
    ClassDB::bind_method(D_METHOD("set_temperature", "value"), &VAWorld::set_temperature);
    ClassDB::bind_method(D_METHOD("get_pressure"), &VAWorld::get_pressure);
    ClassDB::bind_method(D_METHOD("set_pressure", "value"), &VAWorld::set_pressure);
    ClassDB::bind_method(D_METHOD("get_reference_frequency_lf"), &VAWorld::get_reference_frequency_lf);
    ClassDB::bind_method(D_METHOD("set_reference_frequency_lf", "value"), &VAWorld::set_reference_frequency_lf);
    ClassDB::bind_method(D_METHOD("get_reference_frequency_hf"), &VAWorld::get_reference_frequency_hf);
    ClassDB::bind_method(D_METHOD("set_reference_frequency_hf", "value"), &VAWorld::set_reference_frequency_hf);

    // meters_per_unit/speed_of_sound are also forwarded to the process-wide ALManager singleton (see va_world_properties.cpp), same last-write-wins caveat as OpenAL group properties above.
    ADD_GROUP("AirAbsorption", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "meters_per_unit", PROPERTY_HINT_RANGE, "0.0001,1.0,or_greater"), "set_meters_per_unit", "get_meters_per_unit");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound", PROPERTY_HINT_RANGE, "0.0001,1000.0,1,or_greater"), "set_speed_of_sound", "get_speed_of_sound");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "humidity", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_humidity", "get_humidity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature", PROPERTY_HINT_RANGE, "-273.15,100.0,1,or_greater"), "set_temperature", "get_temperature");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pressure", PROPERTY_HINT_RANGE, "0.0,1000000,1,or_greater"), "set_pressure", "get_pressure");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_frequency_lf", PROPERTY_HINT_RANGE, "0.0001,1000,1,or_greater"), "set_reference_frequency_lf", "get_reference_frequency_lf");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_frequency_hf", PROPERTY_HINT_RANGE, "0.0001,20000,1,or_greater"), "set_reference_frequency_hf", "get_reference_frequency_hf");

    ClassDB::bind_method(D_METHOD("get_emitters_outside_the_world_are_muffled"), &VAWorld::get_emitters_outside_the_world_are_muffled);
    ClassDB::bind_method(D_METHOD("set_emitters_outside_the_world_are_muffled", "value"), &VAWorld::set_emitters_outside_the_world_are_muffled);

    ADD_GROUP("Emitters", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emitters_outside_the_world_are_muffled"), "set_emitters_outside_the_world_are_muffled", "get_emitters_outside_the_world_are_muffled");

    ClassDB::bind_method(D_METHOD("get_maximum_concurrency_level"), &VAWorld::get_maximum_concurrency_level);
    ClassDB::bind_method(D_METHOD("set_maximum_concurrency_level", "value"), &VAWorld::set_maximum_concurrency_level);
    ClassDB::bind_method(D_METHOD("get_work_item_count"), &VAWorld::get_work_item_count);
    ClassDB::bind_method(D_METHOD("set_work_item_count", "value"), &VAWorld::set_work_item_count);
    ClassDB::bind_method(D_METHOD("get_pending_shutdown"), &VAWorld::get_pending_shutdown);
    ClassDB::bind_method(D_METHOD("set_pending_shutdown", "value"), &VAWorld::set_pending_shutdown);

    ADD_GROUP("Threading", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_concurrency_level", PROPERTY_HINT_RANGE, "0,32,1,or_greater"), "set_maximum_concurrency_level", "get_maximum_concurrency_level");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "work_item_count", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_work_item_count", "get_work_item_count");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pending_shutdown"), "set_pending_shutdown", "get_pending_shutdown");
    
    ClassDB::bind_method(D_METHOD("get_rendering_enabled"), &VAWorld::get_rendering_enabled);
    ClassDB::bind_method(D_METHOD("set_rendering_enabled", "value"), &VAWorld::set_rendering_enabled);

    ADD_GROUP("Rendering", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rendering_enabled"), "set_rendering_enabled", "get_rendering_enabled");

    // Read-only timing stats (milliseconds) - no ADD_PROPERTY, called directly from GDScript like methods (world.get_main_thread_time()).
    ClassDB::bind_method(D_METHOD("get_main_thread_time"), &VAWorld::get_main_thread_time);
    ClassDB::bind_method(D_METHOD("get_raytracing_time"), &VAWorld::get_raytracing_time);
    ClassDB::bind_method(D_METHOD("get_preparation_time"), &VAWorld::get_preparation_time);
    ClassDB::bind_method(D_METHOD("get_analysis_time"), &VAWorld::get_analysis_time);

    // Read-only GroupedEAX stats, same no-ADD_PROPERTY rationale as the timing stats above.
    ClassDB::bind_method(D_METHOD("get_grouped_eax_count"), &VAWorld::get_grouped_eax_count);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_gain_lf", "index"), &VAWorld::get_grouped_eax_gain_lf);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_gain_hf", "index"), &VAWorld::get_grouped_eax_gain_hf);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_decay_time", "index"), &VAWorld::get_grouped_eax_decay_time);

    // Exports world settings/materials/primitives/emitters to a binary file (vaWorldExport) - callable from GDScript, e.g. wired to a UI button.
    ClassDB::bind_method(D_METHOD("export_to_file", "file_path"), &VAWorld::export_to_file);

    // Exposes the 23 built-in material names and their metadata key to GDScript so the "Vercidium Audio" editor plugin's material dropdown can't drift out of sync.
    ClassDB::bind_static_method("VAWorld", D_METHOD("get_builtin_material_names"), &VAWorld::get_builtin_material_names);
    ClassDB::bind_static_method("VAWorld", D_METHOD("get_material_meta_key"), &VAWorld::get_material_meta_key);
    ClassDB::bind_static_method("VAWorld", D_METHOD("get_use_flat_transmission_meta_key"), &VAWorld::get_use_flat_transmission_meta_key);
}

VAWorld::VAWorld()
{
    // The bounds are an axis-aligned box - scale isn't meaningful (see _validate_property), so don't let an inherited scale skew the rendered gizmo box either.
    set_disable_scale(true);

    // The world should only exist at runtime, not in the editor; world stays nullptr and every other method already null-checks it.
    if (IS_EDITOR_HINT())
    {
        return;
    }

    world = vaWorldCreate();

    // These three calls are guaranteed to pass, no need to check result.
    vaWorldSetCoordinateSystem(world, VACoordinateSystemGodot);
    vaWorldSetUserData(world, this);
    vaWorldSetOnReverbUpdatedCallback(world, &VAWorld::on_reverb_updated_trampoline);

    // These setters handle error checking for us.
    set_position(get_position());
    set_bounds_size(bounds_size);
    set_epsilon(epsilon);
    set_world_is_indoors(world_is_indoors);
    set_maximum_grouped_eax_count(maximum_grouped_eax_count);
    set_meters_per_unit(meters_per_unit);
    set_speed_of_sound(speed_of_sound);
    set_master_volume(master_volume);
    set_distance_model(distance_model);
    set_reverb_only(reverb_only);
    set_humidity(humidity);
    set_temperature(temperature);
    set_pressure(pressure);
    set_reference_frequency_lf(reference_frequency_lf);
    set_reference_frequency_hf(reference_frequency_hf);
    set_emitters_outside_the_world_are_muffled(emitters_outside_the_world_are_muffled);
    set_maximum_concurrency_level(maximum_concurrency_level);
    set_work_item_count(work_item_count);
    set_rendering_enabled(rendering_enabled);

    // ALManager is initialized at GDExtension module-init time, so it's safe to create AL objects here.
    listener_reverb_effect.create();

    set_process(true);
}

VAWorld::~VAWorld()
{
    if (world)
    {
        // Will block the main thread if the user hasn't set pendingShutdown=true first.
        vaWorldWait(world);

        for (::VAEmitter *emitter : pending_emitter_destroys)
        {
            VAResult result = vaEmitterDestroy(emitter);

            // Should never fail as we've called vaWorldWait() above.
            if (result != VA_SUCCESS)
                VA_ERROR("Failed to destroy a pending emitter (VAResult=", VAResultToString(result), ")");
        }
        pending_emitter_destroys.clear();

        VAResult result = vaWorldDestroy(world);

        // Should never fail as we've called vaWorldWait() above.
        if (result != VA_SUCCESS)
            VA_ERROR("Failed to destroy the world (VAResult=", VAResultToString(result), ")");

        world = nullptr;
    }
}

// The bounds are always an axis-aligned box (vaWorldSetPosition/vaWorldSetSize take no rotation/scale), so hide the rest of Node3D's transform and only expose position.
void VAWorld::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("rotation") ||
        p_property.name == StringName("rotation_degrees") ||
        p_property.name == StringName("quaternion") ||
        p_property.name == StringName("basis") ||
        p_property.name == StringName("scale") ||
        p_property.name == StringName("transform") ||
        p_property.name == StringName("rotation_edit_mode") ||
        p_property.name == StringName("rotation_order"))
    {
        p_property.usage = PROPERTY_USAGE_NONE;
    }
}

void VAWorld::_ready()
{
    if (IS_EDITOR_HINT())
    {
        // Scan for unknown vercidium_audio_material values, so warnings appear while editing. get_tree() can be null if this node isn't inside the scene tree yet.
        Node *root = get_tree() ? get_tree()->get_root() : nullptr;

        if (root)
            validate_materials_in_editor(root);

        return;
    }

    // Wait a frame to ensure all children/siblings have been added to the scene.
    callable_mp(this, &VAWorld::init_scene).call_deferred();
}

void VAWorld::_exit_tree()
{
    if (IS_EDITOR_HINT())
        return;

    is_shutting_down = true;

    if (get_tree())
    {
        if (get_tree()->is_connected("node_added", callable_mp(this, &VAWorld::on_node_added)))
            get_tree()->disconnect("node_added", callable_mp(this, &VAWorld::on_node_added));

        if (get_tree()->is_connected("node_removed", callable_mp(this, &VAWorld::on_node_removed)))
            get_tree()->disconnect("node_removed", callable_mp(this, &VAWorld::on_node_removed));

        // get_current_scene() can be null if the tree has no scene loaded (e.g. exiting during shutdown).
        Node *scene_root = get_tree()->get_current_scene();

        if (scene_root)
            remove_primitive(scene_root, true);
    }
}

void VAWorld::_process(double delta)
{
    if (listener)
    {
        ALManager *manager = ALManager::get_singleton();

        if (manager && manager->is_initialized())
        {
            Vector3 position = listener->get_global_position();
            Vector3 forward = -listener->get_global_transform().basis.get_column(2); // Godot's -Z is forward
            Vector3 up = listener->get_global_transform().basis.get_column(1);

            manager->set_listener_position(position);
            manager->set_listener_orientation(forward, up);
        }
    }

    if (world)
    {
        VAResult result = vaWorldUpdate(world);

        if (result != VA_SUCCESS && result != VA_STILL_RUNNING)
        {
            VA_ERROR_NAMED_RESULT(result, "Update failed");
        }
    }
}

bool VAWorld::register_custom_material(va_godot::VACustomMaterial *material)
{
    // Get the lowest id not already claimed by other custom materials.
    int type = FirstCustomMaterialId;

    for (const auto &kvp : custom_materials)
        if (kvp.first >= type)
            type = kvp.first + 1;

    material->set_material_type(type);
    custom_materials[type] = material;
    return true;
}

void VAWorld::register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener)
{
    VAResult result = vaWorldAddEmitter(world, emitter->get_handle());

    switch (result)
    {
        case VA_SUCCESS:
            break;

        case VA_ALREADY_EXISTS:
            VA_ERROR_NAMED("Failed to register emitter '", emitter->get_name(), "' as it is already added to this VAWorld.");
            break;

        case VA_WORLD_CONFLICT:
            VA_ERROR_NAMED("Failed to register emitter '", emitter->get_name(), "' as it is already added to a different VAWorld.");
            break;

        default:
            VA_ERROR_NAMED_RESULT(result, "Failed to register emitter '", emitter->get_name(), "'.");
            break;
    }

    if (is_main_listener)
    {
        if (!listener)
        {
            listener = emitter;

            // Wire up any emitters that registered before this listener existed, instead of leaving them permanently untargeted.
            for (va_godot::VAEmitter *pending_target : pending_targets)
                listener->add_target(pending_target);

            pending_targets.clear();
        }
        else
            VA_WARN_NAMED("This world can only have one VAListener node. Current listener: '", listener->get_name(), "' Second listener: '", emitter->get_name(), "'");

        return;
    }

    if (!listener)
    {
        // The scene added this emitter before the main listener node - hold onto it and add it as a target once the listener registers, instead of dropping it.
        pending_targets.push_back(emitter);

        return;
    }

    listener->add_target(emitter);
}

void VAWorld::unregister_pending_target(va_godot::VAEmitter *emitter)
{
    pending_targets.erase(std::remove(pending_targets.begin(), pending_targets.end(), emitter), pending_targets.end());
}

void VAWorld::unregister_listener(va_godot::VAEmitter *emitter)
{
    if (listener == emitter)
    {
        listener = nullptr;

        // This node may come back (e.g. scene reload), so let a future missing-listener state warn again.
        warned_missing_listener = false;
    }
}

void VAWorld::on_reverb_updated_trampoline(::VAWorld *world)
{
    VAWorld *self = static_cast<VAWorld *>(vaWorldGetUserData(world));

    if (self)
    {
        self->on_reverb_updated();
    }
}

static VAEAXReverbParams CopyReverbParams(const VAEAXReverb *eax)
{
    VAEAXReverbParams params;
    params.density = 0.5f; // hardcoded per openal-soft issue #1229 (static when updated live), matching VAWorldReverb.cs's CopyReverb
    params.diffusion = eax->diffusion;
    params.gain = 1.0f; // gainLF and gainHF control the actual gain
    params.gainHF = eax->gainHF;
    params.gainLF = eax->gainLF;
    params.decayTime = eax->decayTime;
    params.decayHFRatio = eax->decayHFRatio;
    params.decayLFRatio = eax->decayLFRatio;
    params.reflectionsGain = eax->reflectionsGain;
    params.reflectionsDelay = eax->reflectionsDelay;
    params.lateReverbGain = eax->lateReverbGain;
    params.lateReverbDelay = eax->lateReverbDelay;
    params.echoTime = eax->echoTime;
    params.echoDepth = eax->echoDepth;
    params.modulationTime = eax->modulationTime;
    params.modulationDepth = eax->modulationDepth;
    params.airAbsorptionGainHF = eax->airAbsorptionGainHF;
    params.hfReference = eax->hfReference;
    params.lfReference = eax->lfReference;
    params.roomRolloffFactor = eax->roomRolloffFactor;
    params.decayHFLimit = eax->decayHFLimit;

    return params;
}

void VAWorld::on_reverb_updated()
{
    if (!listener || !listener->get_handle())
    {
        // Don't warn during teardown - a reverb update can still be in flight after the listener node has unregistered but before this VAWorld node is destroyed.
        if (!warned_missing_listener && !is_shutting_down)
        {
            VA_WARN_NAMED("Has no VAListener node, so reverb cannot be updated. Add a VAListener node to this scene.");
            warned_missing_listener = true;
        }

        return;
    }

    VAEAXReverb *eax = vaEmitterGetEAX(listener->get_handle());

    if (eax)
    {
        listener_reverb_effect.set_params(CopyReverbParams(eax));
    }

    int grouped_eax_count = vaWorldGetGroupedEAXCount(world);
    const VAEAXReverb **grouped_eax = vaWorldGetGroupedEAX(world);

    for (int i = 0; i < grouped_eax_count; i++)
    {
        if ((int)grouped_reverb_effects.size() <= i)
        {
            std::unique_ptr<ALReverbEffect> new_effect = std::make_unique<ALReverbEffect>();
            new_effect->create();
            grouped_reverb_effects.push_back(std::move(new_effect));
        }

        VAEAXReverbParams params = CopyReverbParams(grouped_eax[i]);

        // Blend in this slot's gain/direction relative to the listener, so a grouped zone the listener hasn't raytraced
        // yet holds its last pan rather than snapping to silence/center - vaEAXReverbGetRelative* return NULL for "no entry yet".
        float *relative_gain = vaEAXReverbGetRelativeGain(grouped_eax[i], listener->get_handle());
        VAVector *relative_direction = vaEAXReverbGetRelativeDirection(grouped_eax[i], listener->get_handle());

        if (relative_gain)
        {
            params.effectSlotGain = MIN(1.0f, MAX(0.0f, *relative_gain));
        }

        if (relative_direction)
        {
            // Rotate the raytraced world-space direction into the listener's local space so OpenAL's listener-relative reflections/late-reverb pan vectors point the right way regardless of listener facing.
            Vector3 listener_rotation = listener->get_global_rotation();
            VAVector pan_vector = vaWorldCalculateListenerRelativePan(world, *relative_direction, listener_rotation.x, listener_rotation.y);
            Vector3 pan = FromVAudio(pan_vector);

            params.reflectionsPan[0] = pan.x;
            params.reflectionsPan[1] = pan.y;
            params.reflectionsPan[2] = pan.z;
            params.lateReverbPan[0] = pan.x;
            params.lateReverbPan[1] = pan.y;
            params.lateReverbPan[2] = pan.z;
        }

        grouped_reverb_effects[i]->set_params(params);
    }
}

ALReverbEffect *VAWorld::get_reverb_effect(::VAEmitter *emitter)
{
    if (emitter && vaEmitterGetAffectsGroupedEAX(emitter))
    {
        int grouped_eax_index = vaEmitterGetGroupedEAXIndex(emitter);

        if (grouped_eax_index >= 0)
        {
            if (grouped_eax_index >= (int)grouped_reverb_effects.size())
            {
                VA_WARN(
                    "Emitter has a grouped EAX index of ", grouped_eax_index,
                    " but only ", (int)grouped_reverb_effects.size(), " EAX presets are available.");
                return &listener_reverb_effect;
            }

            return grouped_reverb_effects[grouped_eax_index].get();
        }
    }

    // Doesn't cast reverb rays or affect a grouped EAX zone - falls back to the listener's reverb effect only if this
    // emitter opted in via use_listener_reverb, otherwise no reverb send at all. use_listener_reverb has no SDK-side
    // backing, so it's read off the va_godot::VAEmitter via the user data vaEmitterCreate stashed on the handle.
    if (emitter)
    {
        VAEmitter *self = static_cast<VAEmitter *>(vaEmitterGetUserData(emitter));

        if (self && !self->get_use_listener_reverb())
            return nullptr;
    }

    return &listener_reverb_effect;
}

} // namespace va_godot
