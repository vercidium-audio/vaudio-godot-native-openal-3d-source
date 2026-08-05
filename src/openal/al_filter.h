#pragma once

#include "al_functions.h"

// Owns one EFX lowpass filter object - the native C++ equivalent of
// openal_soft_bindings's managed/ALFilter.cs, scoped to AL_FILTER_LOWPASS
// only (this checklist item). Reverb effect-slot filters are a separate,
// not-yet-done checklist item, but reuse this same class.
class ALFilter
{
private:
    ALuint handle = 0;
    float gain = 0.0f;
    float gain_hf = 0.0f;

public:
    ALFilter() = default;
    ~ALFilter();

    ALFilter(const ALFilter &) = delete;
    ALFilter &operator=(const ALFilter &) = delete;

    // Allocates a new EFX filter object and sets its type to
    // AL_FILTER_LOWPASS. No-ops (handle stays 0) if ALManager::has_efx() is
    // false. Returns false (with a Godot error already logged) on failure.
    bool create(float initial_gain, float initial_gain_hf);

    void destroy();

    // gain is AL_LOWPASS_GAIN, written unclamped (vaudio's muffling output is
    // already 0-1). gain_hf is treated as an absolute HF gain and converted
    // to be relative to gain before being written to AL_LOWPASS_GAINHF,
    // matching ALFilter.cs's SetGain: gainHF/max(0.01, gain), clamped to 1.
    void set_gain(float new_gain, float new_gain_hf);

    ALuint get_handle() const
    {
        return handle;
    }

    float get_gain() const
    {
        return gain;
    }

    float get_gain_hf() const
    {
        return gain_hf;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
