#pragma once

// Windows-only for v1: soft_oal.dll is loaded via LoadLibrary/GetProcAddress since no import .lib is vendored.
#include <windows.h>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

#include "al_filter.h"
#include "al_functions.h"

using namespace godot;

// Owns the OpenAL Soft device/context pair for the whole plugin, native equivalent of ALManager.cs.
// A singleton Object (not a Node, like Godot's own AudioServer), registered in register_types.cpp.
// Heap-allocated with `memnew`, not a static global - GDExtension binding functions aren't bound
// yet at CRT static-init time, so a static Object-derived global would crash.
class ALManager : public Object
{
    GDCLASS(ALManager, Object);

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
    alcGetProcAddressFn alcGetProcAddress_ = nullptr;

    // ALC_SOFT_reopen_device entry point; null if unsupported, reinitialize() falls back to close+recreate.
    alcReopenDeviceSOFTFn alcReopenDeviceSOFT_ = nullptr;

    // ALC_EXT_capture entry points, used by ALCaptureDevice.
    alcCaptureOpenDeviceFn alcCaptureOpenDevice_ = nullptr;
    alcCaptureCloseDeviceFn alcCaptureCloseDevice_ = nullptr;
    alcCaptureStartFn alcCaptureStart_ = nullptr;
    alcCaptureStopFn alcCaptureStop_ = nullptr;
    alcCaptureSamplesFn alcCaptureSamples_ = nullptr;

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

    // AL_SOFT_callback_buffer entry point; null if unsupported, ALStreamBuffer checks before using it.
    alBufferCallbackSOFTFn alBufferCallbackSOFT_ = nullptr;

    bool efx_present = false;

    // Settings pushed in before (re)opening the device - see reinitialize(). 0/empty means driver default.
    // std::string not godot::String: a godot::String member here once crashed the DLL as a static global.
    std::string device_name;
    int max_auxiliary_sends = 0;

    // ALC_MONO_SOURCES/ALC_STEREO_SOURCES - unlike the other settings, not settable at runtime (see ALManager.cs).
    int max_mono_sources = 0;
    int max_stereo_sources = 0;

    int sample_rate = 0;
    bool hrtf_enabled = false;

    // Stored so reinitialize() can re-apply it after a device/context recreation.
    ALenum distance_model = AL_INVERSE_DISTANCE_CLAMPED;

    // AL_METERS_PER_UNIT (EFX), distinct from VAWorld::meters_per_unit which only feeds vaudio's acoustics model.
    float meters_per_unit = 1.0f;

    // Global AL speed of sound, distinct from VAWorld::speed_of_sound which only feeds vaudio's acoustics model.
    float speed_of_sound = 343.0f;

    // Diagnostic toggle: routes every source's direct path through silence_filter so only reverb is audible.
    bool reverb_only = false;

    // Lazily-created shared gain-0 filter for reverb_only, process-wide like ALSource3D.cs's silenceFilter.
    ALFilter silence_filter;

    bool load_library();
    bool resolve_functions();
    bool resolve_efx_functions();
    bool open_device_and_context();
    void unload_library();
    void close_device();

    // Reads device_name/max_auxiliary_sends/sample_rate/hrtf_enabled from Project Settings.
    void read_settings_from_project_settings();

protected:
    static void _bind_methods();

public:
    static ALManager *get_singleton();

    ALManager() = default;
    ~ALManager();

    // Loads soft_oal.dll, resolves the entry points this plugin needs, opens
    // the default playback device, and creates+activates an OpenAL context.
    // Returns false (with a Godot error already logged) on any failure.
    bool initialize();

    // If ALC_SOFT_reopen_device is present, redirects the existing device/context in place, keeping
    // all AL objects valid. Otherwise falls back to a full close_device()+open_device_and_context().
    bool reinitialize(const String &new_device_name, int new_max_auxiliary_sends, int new_sample_rate, bool new_hrtf_enabled);

    // Runtime device-switching entry point for a shipped game's audio settings menu
    bool set_output_device(const String &new_device_name)
    {
        return reinitialize(new_device_name, max_auxiliary_sends, sample_rate, hrtf_enabled);
    }

    // Process-wide master volume multiplier applied on top of every source's own gain.
    void set_master_volume(float value)
    {
        alListenerf_(AL_GAIN, value);
    }

    // Re-applied automatically after reinitialize() recreates the context (per-context state).
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

    // Re-applied automatically after reinitialize() recreates the context (per-context state).
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

    // Re-applied automatically after reinitialize() recreates the context (per-context state).
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

    // Takes effect next time a source's direct filter is (re)applied, not retroactively on active sources.
    void set_reverb_only(bool value)
    {
        reverb_only = value;
    }

    bool get_reverb_only() const
    {
        return reverb_only;
    }

    // Lazily creates the shared silence filter on first use.
    ALuint get_silence_filter_handle()
    {
        if (!silence_filter.is_valid())
            silence_filter.create(0.0f, 0.0f);

        return silence_filter.get_handle();
    }

    // Falls back to ALC_ENUMERATION_EXT if ALC_ENUMERATE_ALL_EXT is unavailable. Can be called before initialize().
    PackedStringArray get_available_devices();

    // Can be called before initialize().
    PackedStringArray get_available_capture_devices();

    // Safe to call more than once, and even if initialize() was never called or failed partway through.
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

    // Buffer entry points, exposed for ALBuffer to call directly.
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

    // ALC_EXT_capture entry points, exposed for ALCaptureDevice. Unlike other accessors here, these don't
    // require is_initialized() - capture devices are independent ALC devices, opened/closed separately.
    alcCaptureOpenDeviceFn alc_capture_open_device() const
    {
        return alcCaptureOpenDevice_;
    }

    alcCaptureCloseDeviceFn alc_capture_close_device() const
    {
        return alcCaptureCloseDevice_;
    }

    alcCaptureStartFn alc_capture_start() const
    {
        return alcCaptureStart_;
    }

    alcCaptureStopFn alc_capture_stop() const
    {
        return alcCaptureStop_;
    }

    alcCaptureSamplesFn alc_capture_samples() const
    {
        return alcCaptureSamples_;
    }

    // Exposed for ALCaptureDevice::update() to query ALC_CAPTURE_SAMPLES
    // (how many frames are currently ready to read) before calling
    // alc_capture_samples().
    alcGetIntegervFn alc_get_integerv() const
    {
        return alcGetIntegerv_;
    }

    // Source entry points - exposed for ALSourceHandle (src/openal/al_source_handle.cpp),
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

    // Checked by ALFilter before creating any filter objects.
    bool has_efx() const
    {
        return efx_present;
    }

    // Filter entry points, exposed for ALFilter.
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

    // Effect/effect-slot entry points, exposed for ALReverbEffect.
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

    // Exposed for ALStreamBuffer. Null if AL_SOFT_callback_buffer wasn't resolved (see has_efx()).
    alBufferCallbackSOFTFn al_buffer_callback_soft() const
    {
        return alBufferCallbackSOFT_;
    }

    // Called once per frame from VAWorld::_process against the scene's listener VAEmitter.
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
