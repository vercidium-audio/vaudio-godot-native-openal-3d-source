#include "al_source_handle.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

ALSourceHandle::~ALSourceHandle()
{
    destroy();
}

bool ALSourceHandle::create()
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        VA_ERROR("ALSourceHandle::create called before the OpenAL device is initialized");
        return false;
    }

    ALuint new_handle = 0;
    manager->al_gen_sources()(1, &new_handle);

    if (new_handle == 0)
    {
        VA_ERROR("alGenSources failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    handle = new_handle;

    // Every buffer this plugin uploads is stereo (ALBuffer::load always uploads AL_FORMAT_STEREO16), and OpenAL Soft's
    // default AL_AUTO spatialize mode leaves stereo sources unspatialized - AL_SOURCE_SPATIALIZE_SOFT forces 3D spatialization regardless of channel count.
    ALManager::get_singleton()->al_sourcei()(handle, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);

    return true;
}

void ALSourceHandle::destroy()
{
    ALManager *manager = ALManager::get_singleton();

    if (handle != 0 && manager)
    {
        ALuint handles[1] = {handle};
        manager->al_delete_sources()(1, handles);
    }

    handle = 0;
}

void ALSourceHandle::set_buffer(ALuint buffer_handle)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_BUFFER, (ALint)buffer_handle);
}

void ALSourceHandle::play()
{
    ALManager::get_singleton()->al_source_play()(handle);
}

void ALSourceHandle::stop()
{
    ALManager::get_singleton()->al_source_stop()(handle);
}

bool ALSourceHandle::is_finished() const
{
    if (looping)
    {
        return false;
    }

    ALint state = AL_STOPPED;
    ALManager::get_singleton()->al_get_sourcei()(handle, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

void ALSourceHandle::set_gain(float gain)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_GAIN, gain);
}

void ALSourceHandle::set_pitch(float pitch)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_PITCH, pitch);
}

void ALSourceHandle::set_looping(bool value)
{
    looping = value;
    ALManager::get_singleton()->al_sourcei()(handle, AL_LOOPING, value ? 1 : 0);
}

void ALSourceHandle::set_relative(bool value)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_SOURCE_RELATIVE, value ? 1 : 0);
}

void ALSourceHandle::set_max_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_MAX_DISTANCE, value);
}

void ALSourceHandle::set_reference_distance(float value)
{
    ALManager::get_singleton()->al_sourcef()(handle, AL_REFERENCE_DISTANCE, value);
}

void ALSourceHandle::set_position(const Vector3 &position)
{
    ALfloat values[3] = {position.x, position.y, position.z};
    ALManager::get_singleton()->al_sourcefv()(handle, AL_POSITION, values);
}

void ALSourceHandle::set_direct_filter(ALuint filter_handle)
{
    ALManager::get_singleton()->al_sourcei()(handle, AL_DIRECT_FILTER, (ALint)filter_handle);
}

void ALSourceHandle::set_reverb_send(ALuint effect_slot_handle, ALuint reverb_filter_handle)
{
    ALManager::get_singleton()->al_source3i()(
        handle, AL_AUXILIARY_SEND_FILTER,
        (ALint)effect_slot_handle, 0, (ALint)reverb_filter_handle);
}
