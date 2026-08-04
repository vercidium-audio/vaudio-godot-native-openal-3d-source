#include "al_reverb.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ALReverbEffect::~ALReverbEffect()
{
    destroy();
}

bool ALReverbEffect::create()
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        VA_ERROR("ALReverbEffect::create called before the OpenAL device is initialized");
        return false;
    }

    if (!manager->has_efx())
    {
        // Not an error - devices without ALC_EXT_EFX just play without
        // reverb. ALManager::resolve_efx_functions already logged why.
        return false;
    }

    ALuint new_effect = 0;
    manager->al_gen_effects()(1, &new_effect);

    if (new_effect == 0)
    {
        VA_ERROR("alGenEffects failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    ALuint new_slot = 0;
    manager->al_gen_auxiliary_effect_slots()(1, &new_slot);

    if (new_slot == 0)
    {
        VA_ERROR("alGenAuxiliaryEffectSlots failed, AL error ", (int)manager->al_get_error()());
        ALuint effects[1] = {new_effect};
        manager->al_delete_effects()(1, effects);
        return false;
    }

    effect_handle = new_effect;
    slot_handle = new_slot;

    manager->al_effecti()(effect_handle, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
    manager->al_auxiliary_effect_sloti()(slot_handle, AL_EFFECTSLOT_EFFECT, (ALint)effect_handle);

    return true;
}

void ALReverbEffect::destroy()
{
    ALManager *manager = ALManager::get_singleton();

    if (manager && manager->has_efx())
    {
        if (slot_handle != 0)
        {
            ALuint slots[1] = {slot_handle};
            manager->al_delete_auxiliary_effect_slots()(1, slots);
        }

        if (effect_handle != 0)
        {
            ALuint effects[1] = {effect_handle};
            manager->al_delete_effects()(1, effects);
        }
    }

    slot_handle = 0;
    effect_handle = 0;
}

void ALReverbEffect::set_params(const VAEAXReverbParams &params)
{
    if (!is_valid())
    {
        return;
    }

    ALManager *manager = ALManager::get_singleton();
    auto effectf = manager->al_effectf();
    auto effecti = manager->al_effecti();
    auto effectfv = manager->al_effectfv();

    effecti(effect_handle, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    effectf(effect_handle, AL_EAXREVERB_DENSITY, params.density);
    effectf(effect_handle, AL_EAXREVERB_DIFFUSION, params.diffusion);
    effectf(effect_handle, AL_EAXREVERB_GAIN, params.gain);
    effectf(effect_handle, AL_EAXREVERB_GAINHF, params.gainHF);
    effectf(effect_handle, AL_EAXREVERB_GAINLF, params.gainLF);
    effectf(effect_handle, AL_EAXREVERB_DECAY_TIME, params.decayTime);
    effectf(effect_handle, AL_EAXREVERB_DECAY_HFRATIO, params.decayHFRatio);
    effectf(effect_handle, AL_EAXREVERB_DECAY_LFRATIO, params.decayLFRatio);
    effectf(effect_handle, AL_EAXREVERB_REFLECTIONS_GAIN, params.reflectionsGain);
    effectf(effect_handle, AL_EAXREVERB_REFLECTIONS_DELAY, params.reflectionsDelay);
    effectfv(effect_handle, AL_EAXREVERB_REFLECTIONS_PAN, params.reflectionsPan);
    effectf(effect_handle, AL_EAXREVERB_LATE_REVERB_GAIN, params.lateReverbGain);
    effectf(effect_handle, AL_EAXREVERB_LATE_REVERB_DELAY, params.lateReverbDelay);
    effectfv(effect_handle, AL_EAXREVERB_LATE_REVERB_PAN, params.lateReverbPan);
    effectf(effect_handle, AL_EAXREVERB_ECHO_TIME, params.echoTime);
    effectf(effect_handle, AL_EAXREVERB_ECHO_DEPTH, params.echoDepth);
    effectf(effect_handle, AL_EAXREVERB_MODULATION_TIME, params.modulationTime);
    effectf(effect_handle, AL_EAXREVERB_MODULATION_DEPTH, params.modulationDepth);
    effectf(effect_handle, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, params.airAbsorptionGainHF);
    effectf(effect_handle, AL_EAXREVERB_HFREFERENCE, params.hfReference);
    effectf(effect_handle, AL_EAXREVERB_LFREFERENCE, params.lfReference);
    effectf(effect_handle, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, params.roomRolloffFactor);
    effecti(effect_handle, AL_EAXREVERB_DECAY_HFLIMIT, params.decayHFLimit);

    // Re-bind: effect parameter edits only take effect on the slot once
    // AL_EFFECTSLOT_EFFECT is re-written, per EFX semantics.
    manager->al_auxiliary_effect_sloti()(slot_handle, AL_EFFECTSLOT_EFFECT, (ALint)effect_handle);

    manager->al_auxiliary_effect_slotf()(slot_handle, AL_EFFECTSLOT_GAIN, params.effectSlotGain);
}
