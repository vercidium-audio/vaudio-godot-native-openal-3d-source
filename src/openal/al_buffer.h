#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "al_functions.h"

using namespace godot;

// Owns one OpenAL buffer's worth of fully-decoded PCM data - the native
// C++ equivalent of vaudio-godot-openal's ALBuffer (other/ALBuffer.cs),
// scoped down for now to just decode+upload (no background-thread loading,
// no source-creation helper yet - those are separate, not-yet-done
// checklist items in native_godot_plan.md).
//
// Per architectural decision #6: decoding goes through Godot's own
// AudioStream/AudioStreamPlayback pull-based mix API (works uniformly for
// AudioStreamWAV/AudioStreamOggVorbis/AudioStreamMP3 - anything Godot's own
// import pipeline can decode), rather than a bundled decoder. That's only
// worth revisiting if this turns out to be insufficient (wrong data, missing
// format, streaming needs).
class ALBuffer
{
private:
    ALuint handle = 0;
    int sample_rate = 0;
    double duration_seconds = 0.0;

public:
    ALBuffer() = default;
    ~ALBuffer();

    ALBuffer(const ALBuffer &) = delete;
    ALBuffer &operator=(const ALBuffer &) = delete;

    // Fully decodes p_stream via Godot's AudioStreamPlayback::mix_audio pull
    // API and uploads the result as one OpenAL buffer. Returns false (with a
    // Godot error already logged) if decoding or upload failed - handle()
    // stays 0 in that case, safe to check before use. If this ALBuffer
    // already held a previously-loaded buffer, that one is deleted first
    // (load() may be called more than once on the same instance, e.g.
    // ALSourceNode3D re-decoding after `stream` is reassigned at runtime).
    bool load(const Ref<AudioStream> &p_stream);

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
