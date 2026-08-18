#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "al_functions.h"

#include <utility>
#include <vector>

using namespace godot;

// Owns one OpenAL buffer's worth of fully-decoded PCM data - the native
// C++ equivalent of vaudio-godot-mono-openal-3d's ALBuffer (other/ALBuffer.cs),
// scoped down for now to just decode+upload (no source-creation helper yet -
// that's a separate, not-yet-done checklist item in native_godot_plan.md).
//
// Per architectural decision #6: decoding goes through Godot's own
// AudioStream/AudioStreamPlayback pull-based mix API (works uniformly for
// AudioStreamWAV/AudioStreamOggVorbis/AudioStreamMP3 - anything Godot's own
// import pipeline can decode), rather than a bundled decoder. That's only
// worth revisiting if this turns out to be insufficient (wrong data, missing
// format, streaming needs).
//
// decode()/upload() are split so a long stream's decode can run on a
// WorkerThreadPool task (see ALSourceNode::play()) without touching OpenAL
// off the main thread: decode() only pulls PCM via Godot's playback API
// (thread-safe, no AL calls), upload() does the alGenBuffers/alBufferData
// call and must run on the same thread as the rest of this plugin's AL
// calls.
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

    // Movable so a decoded ALBuffer can be stored directly in a
    // std::vector<ALBuffer> (see ALSourceNode::decoded_buffers) - moving
    // leaves the source with handle 0 so its destructor doesn't also delete
    // the AL buffer the moved-to instance now owns.
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

    // Fully decodes p_stream via Godot's AudioStreamPlayback::mix_audio pull
    // API and uploads the result as one OpenAL buffer. Returns false (with a
    // Godot error already logged) if decoding or upload failed - handle()
    // stays 0 in that case, safe to check before use. If this ALBuffer
    // already held a previously-loaded buffer, that one is deleted first
    // (load() may be called more than once on the same instance, e.g.
    // ALSourceNode3D re-decoding after `stream` is reassigned at runtime).
    // Equivalent to calling decode() then upload() - kept for callers that
    // don't need the two steps split across threads.
    bool load(const Ref<AudioStream> &p_stream);

    // Pulls p_stream's PCM data into an internal buffer via
    // AudioStreamPlayback::mix_audio. Safe to call from a background thread -
    // touches no OpenAL state. Returns false (with a Godot error already
    // logged) if the stream failed to decode. Call upload() afterwards, on
    // the main thread, to turn the result into a usable OpenAL buffer.
    bool decode(const Ref<AudioStream> &p_stream);

    // Uploads the PCM data a prior decode() call pulled as one OpenAL
    // buffer. Must run on the main thread (calls alGenBuffers/alBufferData).
    // Returns false (with a Godot error already logged) if decode() wasn't
    // called first or the upload itself failed - handle() stays 0 either way.
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
