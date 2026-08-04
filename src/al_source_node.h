#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node3d.hpp>

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

    // TODO - lazy decode causes game to hang when ambience first plays (20 minute ogg is expensive to load). Can we decode on background threads and play when it's ready, rather than hanging the main thread?
    // Inspector-facing sound to play - decoded into `decoded_buffer` (and
    // buffer_handle pointed at it) lazily on first play(), rather than
    // eagerly in a setter, since a setter can run before the OpenAL device is
    // initialized (e.g. deserializing a .tscn before _enter_tree/VAWorld
    // exist). Re-decoded if the stream is changed after the first play().
    Ref<AudioStream> stream;
    ALBuffer decoded_buffer;
    Ref<AudioStream> decoded_stream;

    // TODO - what does it mean by 'matches'?
    // Ensures decoded_buffer matches `stream`, uploading it and pointing
    // buffer_handle at it if needed. No-op once already decoded for the
    // current `stream`.
    void ensure_stream_decoded();

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

    // TODO - do this
    // Sets which OpenAL buffer new sources play - see buffer_handle above.
    // No sound-name/resource-loading registry exists yet (deferred, unlike
    // ALSource3D.cs's SoundName + ALManager.TryCreateSource); callers must
    // upload their own ALBuffer and pass its handle directly for now.
    void set_buffer_handle(ALuint value)
    {
        buffer_handle = value;
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
