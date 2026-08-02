#pragma once

// Windows-only for v1 (native_godot_plan.md architectural decision #8) -
// soft_oal.dll is loaded directly via the Win32 LoadLibrary/GetProcAddress
// APIs rather than through an import .lib, since none is vendored.
#include <windows.h>

#include <godot_cpp/variant/vector3.hpp>

#include "al_functions.h"

using namespace godot;

// Owns the OpenAL Soft device/context pair for the whole plugin - the native
// C++ equivalent of vaudio-godot-openal's ALManager (manager/ALManagerDevice.cs),
// scoped down for now to just device+context creation/teardown (no capture
// device, no per-listener property plumbing yet - those are separate,
// not-yet-done checklist items in native_godot_plan.md).
//
// Not a Node - there's only ever one OpenAL device for the whole process, so
// this is a plain singleton constructed/destroyed from register_types.cpp's
// module init/uninit hooks, matching architectural decision #5 (build the
// OpenAL layer directly in this plugin's C++, incrementally).
class ALManager
{
private:
    HMODULE library = nullptr;

    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;

    alcOpenDeviceFn alcOpenDevice_ = nullptr;
    alcCloseDeviceFn alcCloseDevice_ = nullptr;
    alcCreateContextFn alcCreateContext_ = nullptr;
    alcMakeContextCurrentFn alcMakeContextCurrent_ = nullptr;
    alcDestroyContextFn alcDestroyContext_ = nullptr;
    alcGetErrorFn alcGetError_ = nullptr;
    alcGetStringFn alcGetString_ = nullptr;
    alcGetIntegervFn alcGetIntegerv_ = nullptr;
    alcIsExtensionPresentFn alcIsExtensionPresent_ = nullptr;

    alGetErrorFn alGetError_ = nullptr;
    alGetStringFn alGetString_ = nullptr;
    alDistanceModelFn alDistanceModel_ = nullptr;
    alSpeedOfSoundFn alSpeedOfSound_ = nullptr;
    alListenerfFn alListenerf_ = nullptr;
    alListenerfvFn alListenerfv_ = nullptr;

    alGenBuffersFn alGenBuffers_ = nullptr;
    alDeleteBuffersFn alDeleteBuffers_ = nullptr;
    alIsBufferFn alIsBuffer_ = nullptr;
    alBufferDataFn alBufferData_ = nullptr;

    alGenSourcesFn alGenSources_ = nullptr;
    alDeleteSourcesFn alDeleteSources_ = nullptr;
    alIsSourceFn alIsSource_ = nullptr;
    alSourceiFn alSourcei_ = nullptr;
    alSourcefFn alSourcef_ = nullptr;
    alSourcefvFn alSourcefv_ = nullptr;
    alGetSourceiFn alGetSourcei_ = nullptr;
    alSourcePlayFn alSourcePlay_ = nullptr;
    alSourceStopFn alSourceStop_ = nullptr;
    alSource3iFn alSource3i_ = nullptr;

    alGetProcAddressFn alGetProcAddress_ = nullptr;

    alGenFiltersFn alGenFilters_ = nullptr;
    alDeleteFiltersFn alDeleteFilters_ = nullptr;
    alFilteriFn alFilteri_ = nullptr;
    alFilterfFn alFilterf_ = nullptr;

    alGenEffectsFn alGenEffects_ = nullptr;
    alDeleteEffectsFn alDeleteEffects_ = nullptr;
    alEffectiFn alEffecti_ = nullptr;
    alEffectfFn alEffectf_ = nullptr;
    alEffectfvFn alEffectfv_ = nullptr;

    alGenAuxiliaryEffectSlotsFn alGenAuxiliaryEffectSlots_ = nullptr;
    alDeleteAuxiliaryEffectSlotsFn alDeleteAuxiliaryEffectSlots_ = nullptr;
    alAuxiliaryEffectSlotiFn alAuxiliaryEffectSloti_ = nullptr;
    alAuxiliaryEffectSlotfFn alAuxiliaryEffectSlotf_ = nullptr;

    bool efx_present = false;

    bool load_library();
    bool resolve_functions();
    bool resolve_efx_functions();
    void unload_library();

public:
    static ALManager *get_singleton();

    ALManager() = default;
    ~ALManager();

    // Loads soft_oal.dll, resolves the entry points this plugin needs, opens
    // the default playback device, and creates+activates an OpenAL context.
    // Returns false (with a Godot error already logged) on any failure.
    bool initialize();

    // Destroys the context and closes the device, if open. Safe to call more
    // than once and safe to call even if initialize() was never called or
    // failed partway through.
    void shutdown();

    bool is_initialized() const
    {
        return device && context;
    }

    ALCdevice *get_device() const
    {
        return device;
    }

    ALCcontext *get_context() const
    {
        return context;
    }

    // Buffer entry points - exposed for ALBuffer (src/openal/al_buffer.cpp).
    // Not wrapped further since ALManager already owns resolving/validating
    // every entry point; ALBuffer just calls through these directly.
    alGenBuffersFn al_gen_buffers() const
    {
        return alGenBuffers_;
    }

    alDeleteBuffersFn al_delete_buffers() const
    {
        return alDeleteBuffers_;
    }

    alBufferDataFn al_buffer_data() const
    {
        return alBufferData_;
    }

    alGetErrorFn al_get_error() const
    {
        return alGetError_;
    }

    // Source entry points - exposed for ALSource (src/openal/al_source.cpp),
    // same "no further wrapping" rationale as the buffer accessors above.
    alGenSourcesFn al_gen_sources() const
    {
        return alGenSources_;
    }

    alDeleteSourcesFn al_delete_sources() const
    {
        return alDeleteSources_;
    }

    alSourceiFn al_sourcei() const
    {
        return alSourcei_;
    }

    alSourcefFn al_sourcef() const
    {
        return alSourcef_;
    }

    alSourcefvFn al_sourcefv() const
    {
        return alSourcefv_;
    }

    alGetSourceiFn al_get_sourcei() const
    {
        return alGetSourcei_;
    }

    alSourcePlayFn al_source_play() const
    {
        return alSourcePlay_;
    }

    alSourceStopFn al_source_stop() const
    {
        return alSourceStop_;
    }

    alSource3iFn al_source3i() const
    {
        return alSource3i_;
    }

    // True if the ALC_EXT_EFX extension was present on the opened device and
    // the filter object entry points were resolved successfully - checked by
    // ALFilter (src/openal/al_filter.cpp) before creating any filter objects.
    bool has_efx() const
    {
        return efx_present;
    }

    // Filter entry points - exposed for ALFilter (src/openal/al_filter.cpp),
    // same "no further wrapping" rationale as the buffer/source accessors
    // above.
    alGenFiltersFn al_gen_filters() const
    {
        return alGenFilters_;
    }

    alDeleteFiltersFn al_delete_filters() const
    {
        return alDeleteFilters_;
    }

    alFilteriFn al_filteri() const
    {
        return alFilteri_;
    }

    alFilterfFn al_filterf() const
    {
        return alFilterf_;
    }

    // Effect/effect-slot entry points - exposed for ALReverbEffect
    // (src/openal/al_reverb.cpp), same "no further wrapping" rationale as
    // the filter accessors above.
    alGenEffectsFn al_gen_effects() const
    {
        return alGenEffects_;
    }

    alDeleteEffectsFn al_delete_effects() const
    {
        return alDeleteEffects_;
    }

    alEffectiFn al_effecti() const
    {
        return alEffecti_;
    }

    alEffectfFn al_effectf() const
    {
        return alEffectf_;
    }

    alEffectfvFn al_effectfv() const
    {
        return alEffectfv_;
    }

    alGenAuxiliaryEffectSlotsFn al_gen_auxiliary_effect_slots() const
    {
        return alGenAuxiliaryEffectSlots_;
    }

    alDeleteAuxiliaryEffectSlotsFn al_delete_auxiliary_effect_slots() const
    {
        return alDeleteAuxiliaryEffectSlots_;
    }

    alAuxiliaryEffectSlotiFn al_auxiliary_effect_sloti() const
    {
        return alAuxiliaryEffectSloti_;
    }

    alAuxiliaryEffectSlotfFn al_auxiliary_effect_slotf() const
    {
        return alAuxiliaryEffectSlotf_;
    }

    // Pushes the AL listener's position/orientation - called once per frame
    // from VAWorld::_process against the scene's listener VAEmitter, matching
    // vaudio-godot-openal's VAWorldGodot.cs._Process
    // (ALManager.instance.ListenerPosition/Pitch/Yaw = listener.GlobalPosition/
    // Pitch/Yaw). No coordinate remap needed (architectural decision #7 in
    // native_godot_plan.md - same as ALSource::set_position). forward/up are
    // AL_ORIENTATION's two 3-float vectors, matching OpenAL's convention
    // (forward = "at", up = "up").
    void set_listener_position(const Vector3 &position)
    {
        ALfloat values[3] = {position.x, position.y, position.z};
        alListenerfv_(AL_POSITION, values);
    }

    void set_listener_orientation(const Vector3 &forward, const Vector3 &up)
    {
        ALfloat values[6] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
        alListenerfv_(AL_ORIENTATION, values);
    }
};
