#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "al_functions.h"

#include <utility>
#include <vector>

using namespace godot;

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

    bool load(const Ref<AudioStream> &p_stream);

    bool decode(const Ref<AudioStream> &p_stream);

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
