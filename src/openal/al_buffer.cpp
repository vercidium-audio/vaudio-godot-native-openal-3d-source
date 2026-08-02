#include "al_buffer.h"

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
    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] ALBuffer::load called before the OpenAL device is initialized");
        return false;
    }

    if (p_stream.is_null())
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] ALBuffer::load called with a null AudioStream");
        return false;
    }

    if (handle != 0)
    {
        ALuint old_handles[1] = {handle};
        manager->al_delete_buffers()(1, old_handles);
        handle = 0;
    }

    Ref<AudioStreamPlayback> playback = p_stream->instantiate_playback();

    if (playback.is_null())
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] AudioStream failed to instantiate a playback (unsupported/corrupt file?)");
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

    std::vector<int16_t> pcm_data;
    pcm_data.reserve((size_t)std::min<int64_t>(expected_frames, (int64_t)mix_rate * 60) * 2);

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
            pcm_data.push_back(float_to_pcm16(frame.x));
            pcm_data.push_back(float_to_pcm16(frame.y));
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

    if (pcm_data.empty())
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] AudioStream decoded to zero frames");
        return false;
    }

    ALuint new_handle = 0;
    manager->al_gen_buffers()(1, &new_handle);

    if (new_handle == 0)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alGenBuffers failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    // mix_audio always returns interleaved stereo frames regardless of the
    // source stream's own channel count (Godot's mixer upmixes mono
    // internally), so AL_FORMAT_STEREO16 is always correct here.
    manager->al_buffer_data()(
        new_handle,
        AL_FORMAT_STEREO16,
        pcm_data.data(),
        (ALsizei)(pcm_data.size() * sizeof(int16_t)),
        (ALsizei)mix_rate);

    ALenum error = manager->al_get_error()();

    if (error != AL_NO_ERROR)
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] alBufferData failed, AL error ", (int)error);
        ALuint handles[1] = {new_handle};
        manager->al_delete_buffers()(1, handles);
        return false;
    }

    handle = new_handle;
    sample_rate = (int)mix_rate;
    duration_seconds = (double)frames_pulled / mix_rate;

    return true;
}
