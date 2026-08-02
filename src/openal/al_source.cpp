#include "al_source.h"

#include "al_manager.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ALSource::~ALSource()
{
    destroy();
}

bool ALSource::create()
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] ALSource::create called before the OpenAL device is initialized");
        return false;
    }

    ALuint new_handle = 0;
    manager->al_gen_sources()(1, &new_handle);

    if (new_handle == 0)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alGenSources failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    handle = new_handle;

    // Every buffer this plugin uploads is stereo (ALBuffer::load always
    // uploads AL_FORMAT_STEREO16 - Godot's mix_audio pull API upmixes mono
    // sources internally before we ever see the samples). OpenAL Soft's
    // default AL_AUTO spatialize mode leaves stereo sources unspatialized
    // (direct L/R passthrough with only distance attenuation, no panning),
    // so without this every VASource/VASourceRelative would be audible but
    // never directional. AL_SOFT_source_spatialize (AL_SOURCE_SPATIALIZE_SOFT)
    // forces 3D spatialization regardless of channel count.
    ALManager::get_singleton()->al_sourcei()(handle, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);

    return true;
}

void ALSource::destroy()
{
    ALManager *manager = ALManager::get_singleton();

    if (handle != 0 && manager)
    {
        ALuint handles[1] = {handle};
        manager->al_delete_sources()(1, handles);
    }

    handle = 0;
}

void ALSource::set_buffer(ALuint buffer_handle)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_BUFFER, (ALint)buffer_handle);
}

void ALSource::play()
{
    ALManager::get_singleton()->al_source_play()(handle);
}

void ALSource::stop()
{
    ALManager::get_singleton()->al_source_stop()(handle);
}

bool ALSource::is_finished() const
{
    if (looping)
    {
        return false;
    }

    ALint state = AL_STOPPED;
    ALManager::get_singleton()->al_get_sourcei()(handle, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

void ALSource::set_gain(float gain)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_GAIN, gain);
}

void ALSource::set_pitch(float pitch)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_PITCH, pitch);
}

void ALSource::set_looping(bool value)
{
    looping = value;
    ALManager::get_singleton()->al_sourcei()(handle, AL_LOOPING, value ? 1 : 0);
}

void ALSource::set_relative(bool value)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_SOURCE_RELATIVE, value ? 1 : 0);
}

void ALSource::set_max_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_MAX_DISTANCE, value);
}

void ALSource::set_reference_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_REFERENCE_DISTANCE, value);
}

void ALSource::set_position(const Vector3 &position)
{
    ALfloat values[3] = {position.x, position.y, position.z};
    ALManager::get_singleton()->al_sourcefv()(handle, AL_POSITION, values);
}

void ALSource::set_direct_filter(ALuint filter_handle)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_DIRECT_FILTER, (ALint)filter_handle);
}

void ALSource::set_reverb_send(ALuint effect_slot_handle, ALuint reverb_filter_handle)
{
    ALManager::get_singleton()->al_source3i()(
        handle, AL_AUXILIARY_SEND_FILTER,
        (ALint)effect_slot_handle, 0, (ALint)reverb_filter_handle);
}
