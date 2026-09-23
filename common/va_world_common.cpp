#include "va_world.h"

#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "va_conversions.h"
#include "va_custom_material.h"
#include "va_emitter.h"
#include "va_engine_util.h"

#include <algorithm>

namespace va_godot
{

// Smallest id reserved for custom (non-built-in) materials - matches vaudio.h's VAMaterialType comment and vaudio-unreal's FirstCustomMaterialId.
static constexpr int FirstCustomMaterialId = 1000;

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

            // Set up the sources that were created before the listener existed
            wire_pending_targets();
        }
        else
            VA_WARN_NAMED("This world can only have one VAListener node. Current listener: '", listener->get_name(), "' Second listener: '", emitter->get_name(), "'");

        return;
    }

    // Keep track of all emitters
    registered_emitters.push_back(emitter);

    if (listener)
    {
        listener->add_target(emitter);
        return;
    }

    // If this node was added before the VAlistener was created, we need to defer-process all sources/emitters later
    if (!wire_pending_targets_queued)
    {
        wire_pending_targets_queued = true;
        callable_mp(this, &VAWorld::wire_pending_targets).call_deferred();
    }
}

void VAWorld::wire_pending_targets()
{
    wire_pending_targets_queued = false;

    if (!listener)
        return;

    for (va_godot::VAEmitter *emitter : registered_emitters)
    {
        // Skip a source whose SDK handle has already been torn down (raytrace_once removal, pending destroy) but whose node is still briefly in registered_emitters.
        if (emitter->get_handle())
            listener->add_target(emitter);
    }
}

void VAWorld::unregister_pending_target(va_godot::VAEmitter *emitter)
{
    registered_emitters.erase(std::remove(registered_emitters.begin(), registered_emitters.end(), emitter), registered_emitters.end());
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

VAEAXReverbParams CopyReverbParams(const VAEAXReverb *eax)
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

    if (emitter)
    {
        VAEmitter *self = static_cast<VAEmitter *>(vaEmitterGetUserData(emitter));

        if (self && !self->get_use_listener_reverb())
            return nullptr;
    }

    return &listener_reverb_effect;
}

} // namespace va_godot
