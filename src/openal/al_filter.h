#pragma once

#include "al_functions.h"

// Owns one EFX lowpass filter object, native equivalent of ALFilter.cs, scoped to AL_FILTER_LOWPASS only.
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

    // Allocates a new EFX filter object and sets its type to AL_FILTER_LOWPASS. No-ops (handle stays 0) if ALManager::has_efx() is false.
    bool create(float initial_gain, float initial_gain_hf);

    void destroy();

    // gain is AL_LOWPASS_GAIN, written unclamped. gain_hf is an absolute HF gain converted relative to gain before writing to AL_LOWPASS_GAINHF, matching ALFilter.cs's SetGain.
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
