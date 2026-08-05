#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_randomizer.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>

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

    // Inspector-facing sound to play - decoded into `decoded_buffer` (and
    // buffer_handle pointed at it) lazily on first play(), rather than
    // eagerly in a setter, since a setter can run before the OpenAL device is
    // initialized (e.g. deserializing a .tscn before _enter_tree/VAWorld
    // exist). Re-decoded if the stream is changed after the first play().
    //
    // Decoding a long stream (e.g. a 20 minute ambience ogg) is expensive, so
    // it runs on a WorkerThreadPool task (decode_task_id) rather than
    // blocking play()'s caller - ALBuffer::decode() only pulls PCM (safe off
    // the main thread), then _process() notices the task finished and calls
    // ALBuffer::upload() (the actual alGenBuffers/alBufferData call) on the
    // main thread, starting playback afterwards if play() was requested
    // while the decode was still in flight.
    Ref<AudioStream> stream;
    ALBuffer decoded_buffer;
    Ref<AudioStream> decoded_stream;

    WorkerThreadPool::TaskID decode_task_id = WorkerThreadPool::INVALID_TASK_ID;
    Ref<AudioStream> decoding_stream;
    bool decode_succeeded = false;
    bool play_requested = false;

    // Starts a background decode of `stream` if one isn't already in flight
    // for it. Returns true if a decode is in flight or `stream` is already
    // decoded (i.e. play() should proceed), false if `stream` is null or
    // decoding failed to even start.
    bool ensure_stream_decode_started();

    // WorkerThreadPool::add_native_task entry point - runs on a worker
    // thread, only touches decoded_buffer.decode() (pure CPU, no OpenAL
    // calls) and decode_succeeded. Both are only read/written on the main
    // thread after is_task_completed() reports true (see
    // poll_decode_task()), so there's no concurrent access to race.
    static void decode_stream_task(void *userdata);

    // Polled every frame from _process(): once decode_task_id completes,
    // uploads the decoded PCM to OpenAL and, if play() was called while the
    // decode was in flight, starts playback.
    void poll_decode_task();

    // Actually creates and starts an ALSource against the current
    // buffer_handle - the part of play() that used to run right after a
    // (blocking) decode, now also called from poll_decode_task() once a
    // background decode finishes.
    bool start_playing();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

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

    // Inspector-facing stream. Decoding is deferred to the first play() (see ensure_stream_decoded).
    Ref<AudioStream> get_stream() const
    {
        return stream;
    }

    void set_stream(const Ref<AudioStream> &value)
    {
        stream = value;
    }

protected:
    std::vector<std::unique_ptr<ALSource>> &get_sources()
    {
        return sources;
    }
};
