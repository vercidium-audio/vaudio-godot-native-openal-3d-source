#pragma once

#include <godot_cpp/variant/vector3.hpp>

#include "al_functions.h"

using namespace godot;

// Owns one OpenAL source (a single play instance) - the native C++
// equivalent of openal_soft_bindings's managed/ALSource.cs, scoped down for
// now to play/stop/position/gain per this checklist item (native_godot_plan.md
// "Build per-source playback"). Filter/reverb send plumbing (ALSource.SetFilter
// in the C# reference) is deliberately left for the separate,
// not-yet-done EFX checklist items.
class ALSourceHandle
{
private:
    ALuint handle = 0;
    bool looping = false;

public:
    ALSourceHandle() = default;
    ~ALSourceHandle();

    ALSourceHandle(const ALSourceHandle &) = delete;
    ALSourceHandle &operator=(const ALSourceHandle &) = delete;

    // Allocates a new OpenAL source via alGenSources. Returns false (with a
    // Godot error already logged) on failure - handle() stays 0 in that case.
    bool create();

    void destroy();

    void set_buffer(ALuint buffer_handle);
    void play();
    void stop();

    // Matches ALSource.cs's Finished(): looping sources are never
    // considered finished, otherwise true once AL_SOURCE_STATE reports
    // AL_STOPPED.
    bool is_finished() const;

    void set_gain(float gain);
    void set_pitch(float pitch);
    void set_looping(bool value);
    void set_relative(bool value);
    void set_max_distance(float value);
    void set_reference_distance(float value);
    void set_position(const Vector3 &position);

    // Attaches (or clears, if filter_handle is 0/AL_FILTER_NULL) an EFX
    // lowpass filter to this source's dry/direct path - the native
    // equivalent of ALSource.cs's SetFilter(directFilter) call.
    void set_direct_filter(ALuint filter_handle);

    // Routes this source's wet/reverb send (aux send index 0) to the given
    // effect slot, optionally through a lowpass filter (0/AL_FILTER_NULL for
    // no filtering, matching VASource.cs's fullReverb=true convention of
    // sending the clean signal to the reverb effect). The native equivalent
    // of ALSource.cs's SetFilter(reverbFilter) call.
    void set_reverb_send(ALuint effect_slot_handle, ALuint reverb_filter_handle = 0);

    ALuint get_handle() const
    {
        return handle;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
