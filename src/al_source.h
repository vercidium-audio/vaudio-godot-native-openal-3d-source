#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "openal/al_buffer.h"
#include "openal/al_filter.h"
#include "openal/al_reverb.h"

#include <memory>
#include <vector>

using namespace godot;

class ALSourceHandle;

class ALSource : public Node3D
{
    GDCLASS(ALSource, Node3D);

private:
    std::vector<std::unique_ptr<ALSourceHandle>> sources;

protected:
    static void _bind_methods();

    ALuint buffer_handle = 0;

    TypedArray<AudioStream> streams;

    std::vector<ALBuffer> decoded_buffers;
    std::vector<Ref<AudioStream>> decoded_streams;

    std::vector<Ref<AudioStream>> pending_streams;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    ALBuffer decoding_buffer;
    bool decode_succeeded = false;
    bool play_requested = false;

    int last_played_index = -1;

    bool ensure_stream_decode_started();

    static void decode_stream_task(void *userdata);

    void poll_decode_task();

    int pick_stream_index() const;

    bool start_playing();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    float pitch_randomness = 1.0f;

    float volume_randomness_db = 0.0f;

    bool playback_no_repeat = true;

    virtual void configure_source(ALSourceHandle &source) = 0;

public:
    ALSource();
    ~ALSource();

    void _ready() override;

    void _process(double delta) override;

    ALFilter filter;

    ALReverbEffect *effect = nullptr;

    virtual bool play();

    void stop();

    bool is_playing() const;

    void update_filter(float new_gain, float new_gain_hf, bool fullReverb = false);

    void set_gain(float value);
    void set_pitch(float value);
    void set_looping(bool value);
    void set_autoplay(bool value);

    float get_gain() const
    {
        return gain;
    }

    float get_pitch() const
    {
        return pitch;
    }

    bool get_looping() const
    {
        return looping;
    }

    bool get_autoplay() const
    {
        return autoplay;
    }

    float get_pitch_randomness() const
    {
        return pitch_randomness;
    }

    void set_pitch_randomness(float value)
    {
        pitch_randomness = value;
    }

    float get_volume_randomness_db() const
    {
        return volume_randomness_db;
    }

    void set_volume_randomness_db(float value)
    {
        volume_randomness_db = value;
    }

    bool get_playback_no_repeat() const
    {
        return playback_no_repeat;
    }

    void set_playback_no_repeat(bool value)
    {
        playback_no_repeat = value;
    }

    TypedArray<AudioStream> get_streams() const
    {
        return streams;
    }

    void set_streams(const TypedArray<AudioStream> &value)
    {
        streams = value;
    }

    // Script-only alias for `streams` matching AudioStreamPlayer3D's single `stream` field (see va_conversion_plugin.cpp's matching remap).
    Ref<AudioStream> get_stream() const
    {
        return streams.is_empty() ? Ref<AudioStream>() : Ref<AudioStream>(streams[0]);
    }

    void set_stream(const Ref<AudioStream> &value)
    {
        TypedArray<AudioStream> value_array;
        value_array.push_back(value);
        streams = value_array;
    }

    // Script-only alias for `pitch` matching AudioStreamPlayer3D's `pitch_scale`.
    float get_pitch_scale() const
    {
        return get_pitch();
    }

    void set_pitch_scale(float value)
    {
        set_pitch(value);
    }

    // Script-only alias for `gain` matching AudioStreamPlayer(3D)'s logarithmic `volume_db`; converts via linear_to_db/db_to_linear since `gain` is linear.
    float get_volume_db() const;
    void set_volume_db(float value);

protected:
    std::vector<std::unique_ptr<ALSourceHandle>> &get_sources()
    {
        return sources;
    }
};
