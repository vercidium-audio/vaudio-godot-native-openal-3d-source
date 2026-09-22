#pragma once

// OpenAL Soft is loaded manually (LoadLibrary/GetProcAddress on Windows, dlopen/dlsym elsewhere) rather than linked, since no import lib is vendored.
#ifdef _WIN32
#include <windows.h>
using va_dynamic_library = HMODULE;
#else
using va_dynamic_library = void *;
#endif

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

#include "al_filter.h"
#include "al_functions.h"

using namespace godot;

class ALManager : public Object
{
    GDCLASS(ALManager, Object);

private:
    va_dynamic_library library = nullptr;

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

    alcReopenDeviceSOFTFn alcReopenDeviceSOFT_ = nullptr;

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

    alBufferCallbackSOFTFn alBufferCallbackSOFT_ = nullptr;

    bool efx_present = false;

    std::string device_name;
    int max_auxiliary_sends = 0;

    int max_mono_sources = 0;
    int max_stereo_sources = 0;

    int sample_rate = 0;
    bool hrtf_enabled = false;

    ALenum distance_model = AL_INVERSE_DISTANCE_CLAMPED;

    float meters_per_unit = 1.0f;

    float speed_of_sound = 343.0f;

    bool reverb_only = false;

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

    bool initialize();
    bool reinitialize(const String &new_device_name, int new_max_auxiliary_sends, int new_sample_rate, bool new_hrtf_enabled);

    bool set_output_device(const String &new_device_name)
    {
        return reinitialize(new_device_name, max_auxiliary_sends, sample_rate, hrtf_enabled);
    }

    void set_master_volume(float value)
    {
        alListenerf_(AL_GAIN, value);
    }

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

    void set_reverb_only(bool value)
    {
        reverb_only = value;
    }

    bool get_reverb_only() const
    {
        return reverb_only;
    }

    ALuint get_silence_filter_handle()
    {
        if (!silence_filter.is_valid())
            silence_filter.create(0.0f, 0.0f);

        return silence_filter.get_handle();
    }

    PackedStringArray get_available_devices();

    PackedStringArray get_available_capture_devices();

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

    alcGetIntegervFn alc_get_integerv() const
    {
        return alcGetIntegerv_;
    }

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

    bool has_efx() const
    {
        return efx_present;
    }

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

    alBufferCallbackSOFTFn al_buffer_callback_soft() const
    {
        return alBufferCallbackSOFT_;
    }

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
