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

// Shared base class for spatialised and relative OpenAL sources
class ALSource : public Node3D
{
    GDCLASS(ALSource, Node3D);

private:
    std::vector<std::unique_ptr<ALSourceHandle>> sources;

protected:
    static void _bind_methods();

    // Buffer every new ALSourceHandle created by play() uploads/attaches; subclasses (VASource) set this before calling play().
    ALuint buffer_handle = 0;

    // Inspector-facing pool of sounds to play, one picked at random each play() (see pick_stream_index()). Native replacement
    // for AudioStreamRandomizer (removed: it re-decoded a sub-stream from scratch on every play()) - streams here are cached.
    TypedArray<AudioStream> streams;

    // One decoded+uploaded ALBuffer per entry in `streams`; decoded_streams tracks which Ref<AudioStream> each belongs to so changes can be re-decoded.
    std::vector<ALBuffer> decoded_buffers;
    std::vector<Ref<AudioStream>> decoded_streams;

    // Streams waiting for a decode task; decode_stream_task() pops one at a time so only one background decode is ever in flight per node.
    std::vector<Ref<AudioStream>> pending_streams;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    ALBuffer decoding_buffer;
    bool decode_succeeded = false;
    bool play_requested = false;

    // Index picked by the most recent pick_stream_index() call, so playback_no_repeat can avoid picking it again.
    int last_played_index = -1;

    // Queues a background decode for every entry in `streams` not already decoded/pending/decoding. Returns true if
    // every entry is already decoded (play() should proceed), false if `streams` is empty or a decode just started.
    bool ensure_stream_decode_started();

    // WorkerThreadPool task entry point; only touches decoding_buffer.decode() and decode_succeeded, both read/written
    // on the main thread only after is_task_completed() (see poll_decode_task()), so there's no concurrent access to race.
    static void decode_stream_task(void *userdata);

    // Polled every frame from _process(): once decode_task_id completes, uploads the decoded PCM to OpenAL, starts the
    // next queued decode (if any), and if play() was called while a decode was in flight, starts playback once done.
    void poll_decode_task();

    // Picks a random index into decoded_buffers, honouring playback_no_repeat. Returns -1 if decoded_buffers is empty.
    int pick_stream_index() const;

    // Creates and starts an ALSourceHandle against a randomly-picked decoded buffer, applying pitch/volume randomness -
    // the part of play() that runs right after a decode finishes (either immediately or via poll_decode_task()).
    bool start_playing();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    // Random pitch variation applied on top of `pitch` each play(), as a ratio (1.0 = no variation), matching AudioStreamRandomizer's random_pitch.
    float pitch_randomness = 1.0f;

    // Random volume variation applied on top of `gain` each play(), in dB (0.0 = no variation), matching AudioStreamRandomizer's random_volume_offset_db.
    float volume_randomness_db = 0.0f;

    // When true (default) and streams has more than one entry, the same entry is never picked twice in a row.
    bool playback_no_repeat = true;

    // Hook for subclasses to configure a freshly-created ALSourceHandle before it starts playing
    virtual void configure_source(ALSourceHandle &source) = 0;

public:
    ALSource();
    ~ALSource();

    // Matches AudioStreamPlayer3D's Autoplay: play() is called once this node first becomes ready in a running scene tree. Virtual so VASource can override it.
    void _ready() override;

    // Polls decode_task_id for completion - see poll_decode_task(). Subclasses overriding _process (e.g. ALSource3D) must call this base version.
    void _process(double delta) override;

    // Low pass filter,  lazily created on first UpdateFilter/Play call
    ALFilter filter;

    // Reverb effect, not owned (it is either listener_reverb_effect or grouped_reverb_effects)
    ALReverbEffect *effect = nullptr;

    virtual bool play();

    void stop();

    // True once every source has finished playing
    bool is_playing() const;

    // Creates/updates the low pass filter, then applies the filter and reverb effects
    // fullReverb controls whether a filter is applied on the sound going into the filter
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

    // Decoding is deferred to the first play() (see ensure_stream_decode_started()).
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
