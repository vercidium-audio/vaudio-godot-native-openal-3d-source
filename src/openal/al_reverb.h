#pragma once

#include "al_functions.h"

// Owns one EFX EAXREVERB effect object bound to one auxiliary effect slot -
// the native C++ equivalent of openal_soft_bindings's managed/
// ALReverbEffect.cs. Used both for the single global listener slot and for
// each entry in VAWorld's grouped-EAX pool (VAWorldReverb.cs's
// groupedReverbEffects, keyed by vaEmitterGetGroupedEAXIndex) - see
// VAWorld::on_reverb_updated.
//
// Parameters are set via set_params (mirrors vaudio.h's VAEAXReverb struct
// field-for-field) rather than exposing 20+ individual setters, since the
// SDK always hands back the whole struct at once (vaEmitterGetEAX).
struct VAEAXReverbParams
{
    float density = 1.0f;
    float diffusion = 1.0f;
    float gain = 1.0f;
    float gainHF = 1.0f;
    float gainLF = 1.0f;
    float decayTime = 1.0f;
    float decayHFRatio = 1.0f;
    float decayLFRatio = 1.0f;
    float reflectionsGain = 1.0f;
    float reflectionsDelay = 0.0f;
    float reflectionsPan[3] = {0.0f, 0.0f, 0.0f};
    float lateReverbGain = 1.0f;
    float lateReverbDelay = 0.0f;
    float lateReverbPan[3] = {0.0f, 0.0f, 0.0f};
    float echoTime = 0.25f;
    float echoDepth = 0.0f;
    float modulationTime = 0.25f;
    float modulationDepth = 0.0f;
    float airAbsorptionGainHF = 0.994f;
    float hfReference = 5000.0f;
    float lfReference = 250.0f;
    float roomRolloffFactor = 0.0f;
    int decayHFLimit = 1; // AL_TRUE

    // Grouped-EAX only (VAWorldReverb.cs's CopyReverb "isGroupedEAX" branch) -
    // the aux effect slot's own send gain (AL_EFFECTSLOT_GAIN), driven by
    // vaEAXReverbGetRelativeGain(eax, listener) so a grouped zone fades out
    // for a listener that hasn't raytraced it (or is outside its relative-
    // reverb blend range). Stays 1.0 for the single global listener slot.
    float effectSlotGain = 1.0f;
};

class ALReverbEffect
{
private:
    ALuint effect_handle = 0;
    ALuint slot_handle = 0;

public:
    ALReverbEffect() = default;
    ~ALReverbEffect();

    ALReverbEffect(const ALReverbEffect &) = delete;
    ALReverbEffect &operator=(const ALReverbEffect &) = delete;

    // Allocates an EFX effect object (type AL_EFFECT_EAXREVERB) and an
    // auxiliary effect slot, and binds the two together. No-ops (handles
    // stay 0) if ALManager::has_efx() is false. Returns false (with a Godot
    // error already logged) on failure.
    bool create();

    void destroy();

    // Writes every AL_EAXREVERB_* parameter from params, then re-binds
    // AL_EFFECTSLOT_EFFECT (required by EFX semantics - parameter edits on
    // an effect object only take effect on the slot once the slot's
    // AL_EFFECTSLOT_EFFECT is re-written). Matches ALReverbEffect.cs's
    // Update(). Note: matching VAWorldReverb.cs's CopyReverb, callers should
    // hardcode params.density to 0.5f rather than the SDK's live value -
    // see openal-soft issue #1229 (density causes static when updated in
    // real time).
    void set_params(const VAEAXReverbParams &params);

    ALuint get_slot_handle() const
    {
        return slot_handle;
    }

    bool is_valid() const
    {
        return effect_handle != 0 && slot_handle != 0;
    }
};
