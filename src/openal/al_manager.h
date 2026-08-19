#pragma once

// Windows-only for v1 (native_godot_plan.md architectural decision #8) -
// soft_oal.dll is loaded directly via the Win32 LoadLibrary/GetProcAddress
// APIs rather than through an import .lib, since none is vendored.
#include <windows.h>

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

#include "al_filter.h"
#include "al_functions.h"

using namespace godot;

// Owns the OpenAL Soft device/context pair for the whole plugin - the native
// C++ equivalent of vaudio-godot-mono-openal-3d's ALManager (manager/ALManagerDevice.cs),
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

    // Settings ALManagerNode pushes in before (re)opening the device - see
    // reinitialize(). Empty device_name means "default device" (matches
    // initialize()'s original alcOpenDevice(nullptr) behaviour);
    // max_auxiliary_sends of 0 means "leave it to the driver default" (no
    // ALC_MAX_AUXILIARY_SENDS attribute is passed to alcCreateContext).
    //
    // std::string, not godot::String: ALManager's the process-wide global
    // (see register_types.cpp's `static ALManager al_manager`), so this
    // member's default constructor runs at CRT static-init time, before
    // GDExtensionBinding has bound Godot's API (the string/ctor/dtor
    // function pointers godot::String's own constructor calls through
    // aren't populated yet then) - a godot::String member here crashed the
    // DLL's init routine (Win32 error 1114) before any of our own code,
    // even DllMain, ever ran.
    std::string device_name;
    int max_auxiliary_sends = 0;

    // ALC_FREQUENCY attribute passed to alcCreateContext - 0 means "leave the
    // mixing sample rate at the driver default" (matches max_auxiliary_sends'
    // 0-means-default convention above).
    int sample_rate = 0;

    // ALC_HRTF_SOFT attribute passed to alcCreateContext (ALC_SOFT_HRTF
    // extension) - lets the user request HRTF-based binaural rendering
    // instead of the driver's default panning. The driver can still refuse
    // (e.g. no HRTF data for the output device); not checked here.
    bool hrtf_enabled = false;

    // Current AL distance model - applied via alDistanceModel_ in
    // open_device_and_context(). Stored so reinitialize() can re-apply it
    // after a device/context recreation.
    ALenum distance_model = AL_INVERSE_DISTANCE_CLAMPED;

    // AL_METERS_PER_UNIT (EFX) listener property - the OpenAL/EFX-side
    // scale used by air-absorption and reverb decay math, distinct from
    // VAWorld::meters_per_unit which only feeds vaudio's own raytracing/
    // acoustics model. Applied via alListenerf_, so - like master volume -
    // it's per-context state re-applied after reinitialize() recreates the
    // context, not baked into alcCreateContext's attribute list.
    float meters_per_unit = 1.0f;

    // Global AL speed of sound (alSpeedOfSound), used by OpenAL's own
    // Doppler calculation - distinct from VAWorld::speed_of_sound which only
    // feeds vaudio's raytracing/acoustics model. Also per-context state.
    float speed_of_sound = 343.0f;

    // When true, every ALSourceNode's direct/dry path is routed through a
    // gain-0 lowpass filter (silence_filter below) instead of its own muffle
    // filter, so only each source's reverb send is audible - a diagnostic
    // toggle for tuning reverb in isolation. Matches ALManager.cs's
    // ReverbOnly / ALSource3D.cs's silenceFilter.
    bool reverb_only = false;

    // Lazily-created shared AL_LOWPASS_GAIN=0 filter substituted for a
    // source's own direct filter while reverb_only is enabled - process-wide
    // like ALSource3D.cs's static ALFilter silenceFilter, since silence is
    // the same regardless of which source it's applied to.
    ALFilter silence_filter;

    bool load_library();
    bool resolve_functions();
    bool resolve_efx_functions();
    bool open_device_and_context();
    void unload_library();
    void close_device();

public:
    static ALManager *get_singleton();

    ALManager() = default;
    ~ALManager();

    // Loads soft_oal.dll, resolves the entry points this plugin needs, opens
    // the default playback device, and creates+activates an OpenAL context.
    // Returns false (with a Godot error already logged) on any failure.
    bool initialize();

    // Tears down the current device/context (if any) and reopens them using
    // device_name/max_auxiliary_sends/new_sample_rate/new_hrtf_enabled,
    // leaving the resolved function pointers and loaded library alone.
    // Called by VAOpenALSettings::_ready when its exported device/reverb-send
    // settings differ from the defaults initialize() already applied at
    // module load - see register_types.cpp. Every existing AL object
    // (sources, buffers, filters, effects) is invalidated by this - safe only
    // because it's expected to run once, early, before any VASource/VAEmitter
    // has created OpenAL objects against the old device.
    bool reinitialize(const String &new_device_name, int new_max_auxiliary_sends, int new_sample_rate, bool new_hrtf_enabled);

    // Thin forward to alListenerf(AL_GAIN, ...) - the process-wide master
    // volume multiplier applied on top of every source's own gain. Safe to
    // call at any time after initialize() (no context recreation needed).
    void set_master_volume(float value)
    {
        alListenerf_(AL_GAIN, value);
    }

    // Thin forward to alDistanceModel_ - safe to call at any time after
    // initialize() (no context recreation needed). Re-applied automatically
    // after reinitialize() recreates the context, since AL distance model is
    // per-context state.
    void set_distance_model(ALenum value)
    {
        distance_model = value;

        if (alDistanceModel_)
            alDistanceModel_(distance_model);
    }

    ALenum get_distance_model() const
    {
        return distance_model;
    }

    // Thin forward to alListenerf(AL_METERS_PER_UNIT, ...) - safe to call at
    // any time after initialize() (no context recreation needed). Re-applied
    // automatically after reinitialize() recreates the context, since this is
    // per-context listener state.
    void set_meters_per_unit(float value)
    {
        meters_per_unit = value;

        if (alListenerf_)
            alListenerf_(AL_METERS_PER_UNIT, meters_per_unit);
    }

    float get_meters_per_unit() const
    {
        return meters_per_unit;
    }

    // Thin forward to alSpeedOfSound_ - safe to call at any time after
    // initialize() (no context recreation needed). Re-applied automatically
    // after reinitialize() recreates the context, since AL speed of sound is
    // per-context state.
    void set_speed_of_sound(float value)
    {
        speed_of_sound = value;

        if (alSpeedOfSound_)
            alSpeedOfSound_(speed_of_sound);
    }

    float get_speed_of_sound() const
    {
        return speed_of_sound;
    }

    // Enables/disables reverb-only mode - see reverb_only's doc comment.
    // Takes effect the next time a source's direct filter is (re)applied
    // (ALSourceNode::start_playing/update_filter), not retroactively on
    // sources already playing with their normal filter attached.
    void set_reverb_only(bool value)
    {
        reverb_only = value;
    }

    bool get_reverb_only() const
    {
        return reverb_only;
    }

    // Returns the shared gain-0 lowpass filter used for a source's direct
    // path while reverb_only is enabled, lazily creating it on first use (a
    // no-op returning an invalid/0 handle if ALC_EXT_EFX isn't present, same
    // as any other ALFilter). Callers should prefer this over filter.get_handle()
    // for a source's direct filter whenever get_reverb_only() is true.
    ALuint get_silence_filter_handle()
    {
        if (!silence_filter.is_valid())
            silence_filter.create(0.0f, 0.0f);

        return silence_filter.get_handle();
    }

    // Lists every playback device name the current driver reports via the
    // ALC_ENUMERATE_ALL_EXT extension (falls back to the basic
    // ALC_ENUMERATION_EXT list if that's unavailable) - exposed as
    // VAOpenALSettings::get_available_devices for discovering valid
    // device_name values. Can be called before initialize() - opens no
    // device itself, just queries alcGetString(nullptr, ...).
    PackedStringArray get_available_devices();

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
    // vaudio-godot-mono-openal-3d's VAWorldGodot.cs._Process
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
