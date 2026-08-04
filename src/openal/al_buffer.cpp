#include "al_buffer.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_playback.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <vector>

using namespace godot;

// Pull frames from playback in fixed-size chunks rather than one huge
// mix_audio() call - keeps a single temporary allocation bounded regardless
// of how long the source stream is.
static constexpr int32_t MIX_CHUNK_FRAMES = 8192;

static int16_t float_to_pcm16(float sample)
{
    float clamped = std::clamp(sample, -1.0f, 1.0f);
    return (int16_t)(clamped * 32767.0f);
}

ALBuffer::~ALBuffer()
{
    ALManager *manager = ALManager::get_singleton();

    if (handle != 0 && manager)
    {
        ALuint handles[1] = {handle};
        manager->al_delete_buffers()(1, handles);
    }

    handle = 0;
}

bool ALBuffer::load(const Ref<AudioStream> &p_stream)
{
    return decode(p_stream) && upload();
}

bool ALBuffer::decode(const Ref<AudioStream> &p_stream)
{
    pending_pcm_data.clear();
    pending_frames_pulled = 0;

    if (p_stream.is_null())
    {
        VA_ERROR("ALBuffer::decode called with a null AudioStream");
        return false;
    }

    Ref<AudioStreamPlayback> playback = p_stream->instantiate_playback();

    if (playback.is_null())
    {
        VA_ERROR("AudioStream failed to instantiate a playback (unsupported/corrupt file?)");
        return false;
    }

    float mix_rate = AudioServer::get_singleton()->get_mix_rate();
    double length_seconds = p_stream->get_length();

    // Some streams (e.g. procedurally-generated or malformed ones) report a
    // zero/negative length. Fall back to pulling until mix_audio stops
    // returning frames, bounded by a generous frame count so a broken
    // stream can't loop forever.
    int64_t expected_frames = length_seconds > 0.0
        ? (int64_t)(length_seconds * mix_rate) + MIX_CHUNK_FRAMES
        : (int64_t)mix_rate * 60 * 10; // 10 minute safety cap

    playback->start();

    pending_pcm_data.reserve((size_t)std::min<int64_t>(expected_frames, (int64_t)mix_rate * 60) * 2);

    int64_t frames_pulled = 0;

    while (frames_pulled < expected_frames)
    {
        PackedVector2Array chunk = playback->mix_audio(1.0f, MIX_CHUNK_FRAMES);

        if (chunk.size() == 0)
        {
            break;
        }

        for (int64_t i = 0; i < chunk.size(); i++)
        {
            Vector2 frame = chunk[i];
            pending_pcm_data.push_back(float_to_pcm16(frame.x));
            pending_pcm_data.push_back(float_to_pcm16(frame.y));
        }

        frames_pulled += chunk.size();

        // mix_audio returning fewer frames than requested means the stream
        // has ended (matches Godot's documented pull-based mix contract).
        if (chunk.size() < MIX_CHUNK_FRAMES)
        {
            break;
        }
    }

    playback->stop();

    if (pending_pcm_data.empty())
    {
        VA_ERROR("AudioStream decoded to zero frames");
        return false;
    }

    pending_frames_pulled = frames_pulled;
    sample_rate = (int)mix_rate;
    return true;
}

bool ALBuffer::upload()
{
    if (pending_pcm_data.empty())
    {
        VA_ERROR("ALBuffer::upload called with no decoded PCM data - decode() must be called first");
        return false;
    }

    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        VA_ERROR("ALBuffer::upload called before the OpenAL device is initialized");
        return false;
    }

    if (handle != 0)
    {
        ALuint old_handles[1] = {handle};
        manager->al_delete_buffers()(1, old_handles);
        handle = 0;
    }

    ALuint new_handle = 0;
    manager->al_gen_buffers()(1, &new_handle);

    if (new_handle == 0)
    {
        VA_ERROR("alGenBuffers failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    // mix_audio always returns interleaved stereo frames regardless of the
    // source stream's own channel count (Godot's mixer upmixes mono
    // internally), so AL_FORMAT_STEREO16 is always correct here.
    manager->al_buffer_data()(
        new_handle,
        AL_FORMAT_STEREO16,
        pending_pcm_data.data(),
        (ALsizei)(pending_pcm_data.size() * sizeof(int16_t)),
        (ALsizei)sample_rate);

    ALenum error = manager->al_get_error()();

    if (error != AL_NO_ERROR)
    {
        VA_ERROR("alBufferData failed, AL error ", (int)error);
        ALuint handles[1] = {new_handle};
        manager->al_delete_buffers()(1, handles);
        return false;
    }

    handle = new_handle;
    duration_seconds = (double)pending_frames_pulled / sample_rate;

    // Free the PCM copy now that it's been handed to OpenAL (which keeps its
    // own copy) - no need to hold onto it, and it'd otherwise stick around
    // for this ALBuffer's whole lifetime.
    pending_pcm_data.clear();
    pending_pcm_data.shrink_to_fit();

    return true;
}
