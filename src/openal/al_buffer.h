#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "al_functions.h"

#include <utility>
#include <vector>

using namespace godot;

// Owns one OpenAL buffer's worth of fully-decoded PCM data, native equivalent of ALBuffer.cs.
// Decoding goes through Godot's own AudioStream/AudioStreamPlayback pull-based mix API rather than a bundled decoder.
// decode()/upload() are split so a long stream's decode can run on a WorkerThreadPool task (see ALSource::play())
// without touching OpenAL off the main thread: decode() only pulls PCM (thread-safe), upload() does the AL calls.
class ALBuffer
{
private:
    ALuint handle = 0;
    int sample_rate = 0;
    double duration_seconds = 0.0;

    std::vector<int16_t> pending_pcm_data;
    int64_t pending_frames_pulled = 0;

public:
    ALBuffer() = default;
    ~ALBuffer();

    ALBuffer(const ALBuffer &) = delete;
    ALBuffer &operator=(const ALBuffer &) = delete;

    // Movable so a decoded ALBuffer can be stored directly in a std::vector (see ALSource::decoded_buffers); moving leaves the source's handle at 0.
    ALBuffer(ALBuffer &&other) noexcept
    {
        *this = std::move(other);
    }

    ALBuffer &operator=(ALBuffer &&other) noexcept
    {
        if (this != &other)
        {
            handle = other.handle;
            sample_rate = other.sample_rate;
            duration_seconds = other.duration_seconds;
            pending_pcm_data = std::move(other.pending_pcm_data);
            pending_frames_pulled = other.pending_frames_pulled;

            other.handle = 0;
        }

        return *this;
    }

    // Decodes p_stream and uploads it as one OpenAL buffer; equivalent to decode()+upload(), for callers that don't need the two steps split across threads.
    bool load(const Ref<AudioStream> &p_stream);

    // Pulls p_stream's PCM data via AudioStreamPlayback::mix_audio. Safe to call from a background thread - touches no OpenAL state. Call upload() afterwards on the main thread.
    bool decode(const Ref<AudioStream> &p_stream);

    // Uploads the PCM data a prior decode() call pulled, as one OpenAL buffer. Must run on the main thread (calls alGenBuffers/alBufferData).
    bool upload();

    ALuint get_handle() const
    {
        return handle;
    }

    int get_sample_rate() const
    {
        return sample_rate;
    }

    double get_duration_seconds() const
    {
        return duration_seconds;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
