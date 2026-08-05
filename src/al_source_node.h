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

class ALSource;

// Shared base class for spatialised and relative OpenAL sources
class ALSourceNode : public Node3D
{
    GDCLASS(ALSourceNode, Node3D);

private:
    std::vector<std::unique_ptr<ALSource>> sources;

protected:
    static void _bind_methods();

    // The buffer every new ALSource created by play() uploads/attaches -
    // subclasses (VASource) set this before calling play(). Mirrors
    // ALSource3D.cs's Play() calling ALManager.instance.TryCreateSource(
    // SoundName, ...) - this native port has no sound-name/resource-loading
    // registry yet, so the buffer handle is supplied directly instead.
    ALuint buffer_handle = 0;

    // Inspector-facing pool of sounds to play - one entry is picked at random
    // each play() (see pick_stream_index()). A single fixed sound is just a
    // one-entry array. Native replacement for pointing an AudioStreamPlayer's
    // stream at an AudioStreamRandomizer (removed: it re-decoded a sub-stream
    // from scratch on every single play(), which is slow for frequently-firing
    // sounds like impacts) - streams here are decoded once each and cached.
    TypedArray<AudioStream> streams;

    // One decoded+uploaded ALBuffer per entry in `streams` - decoded once
    // each and cached. decoded_streams mirrors which Ref<AudioStream> each
    // decoded_buffers entry belongs to, so a change to `streams` can be
    // detected and re-decoded.
    std::vector<ALBuffer> decoded_buffers;
    std::vector<Ref<AudioStream>> decoded_streams;

    // Streams still waiting for a decode task (see ensure_stream_decode_started());
    // decode_stream_task() pops and decodes one at a time onto decode_task_id
    // so only one background decode is ever in flight per node.
    std::vector<Ref<AudioStream>> pending_streams;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    ALBuffer decoding_buffer;
    bool decode_succeeded = false;
    bool play_requested = false;

    // Index into decoded_buffers picked by the most recent pick_stream_index()
    // call, so playback_mode's no-repeat rule can avoid picking it again.
    int last_played_index = -1;

    // Queues a background decode for every entry in `streams` not already in
    // decoded_streams/pending_streams/decoding_stream. Returns true if every
    // entry is already decoded (i.e. play() should proceed), false if
    // `streams` is empty or a decode just started/is in flight.
    bool ensure_stream_decode_started();

    // WorkerThreadPool::add_native_task entry point - runs on a worker
    // thread, only touches decoding_buffer.decode() (pure CPU, no OpenAL
    // calls) and decode_succeeded. Both are only read/written on the main
    // thread after is_task_completed() reports true (see
    // poll_decode_task()), so there's no concurrent access to race.
    static void decode_stream_task(void *userdata);

    // Polled every frame from _process(): once decode_task_id completes,
    // uploads the decoded PCM to OpenAL, starts the next queued decode (if
    // any), and if play() was called while any decode was in flight, starts
    // playback once the whole batch is done.
    void poll_decode_task();

    // Picks a random index into decoded_buffers, honouring playback_no_repeat
    // (skip whichever index played last time, when there's more than one
    // choice). Returns -1 if decoded_buffers is empty.
    int pick_stream_index() const;

    // Actually creates and starts an ALSource against a randomly-picked
    // decoded buffer, applying pitch_randomness/volume_randomness_db on top
    // of pitch/gain - the part of play() that used to run right after a
    // (blocking) decode, now also called from poll_decode_task() once a
    // background decode finishes.
    bool start_playing();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    // Random pitch variation applied on top of `pitch` each play(), as a
    // ratio (1.0 = no variation) - matches AudioStreamRandomizer's
    // random_pitch: actual pitch is randf_range(pitch / pitch_randomness,
    // pitch * pitch_randomness).
    float pitch_randomness = 1.0f;

    // Random volume variation applied on top of `gain` each play(), in dB
    // (0.0 = no variation) - matches AudioStreamRandomizer's
    // random_volume_offset_db: actual gain is gain * db_to_linear(randf_range(
    // -volume_randomness_db, volume_randomness_db)).
    float volume_randomness_db = 0.0f;

    // When true (default, matches AudioStreamRandomizer's default
    // PLAYBACK_RANDOM_NO_REPEATS) and streams has more than one entry, the
    // same entry is never picked twice in a row.
    bool playback_no_repeat = true;

    // Hook for subclasses to configure a freshly-created ALSource before it starts playing
    virtual void configure_source(ALSource &source) = 0;

public:
    ALSourceNode();
    ~ALSourceNode();

    // Matches AudioStreamPlayer3D's Autoplay: play() is called once when this
    // node first becomes ready in a running (non-editor) scene tree. Virtual
    // so VASource can override its own _ready and still get this behaviour
    // (its play() defers to raytracing completing, so the call here just arms
    // it the same way a manual play() call would).
    void _ready() override;

    // Polls decode_task_id for completion - see poll_decode_task(). Subclasses
    // overriding _process (e.g. ALSourceNode3D) must call this base version.
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

    // Inspector-facing pool of streams to pick randomly from each play() -
    // a single fixed sound is just a one-entry array. Decoding is deferred to
    // the first play() (see ensure_stream_decode_started()).
    TypedArray<AudioStream> get_streams() const
    {
        return streams;
    }

    void set_streams(const TypedArray<AudioStream> &value)
    {
        streams = value;
    }

protected:
    std::vector<std::unique_ptr<ALSource>> &get_sources()
    {
        return sources;
    }
};
