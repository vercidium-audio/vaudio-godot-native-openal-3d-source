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

// Shared base for ALSourceNode3D (positional/spatialized) and
// ALSourceNodeRelative (AL_SOURCE_RELATIVE, always heard at the listener -
// e.g. UI/footstep-style or ambience sounds) - everything here is agnostic to
// how a subclass positions its sources. Owns a list of concurrently-live
// ALSource instances (not just one) so overlapping one-shot plays don't cut
// each other off, matching godot-openal's ALSource3D.cs's own `sources` list -
// a Play() call always starts a new ALSource and appends it, never replaces
// an existing one.
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
    Ref<AudioStream> stream;
    ALBuffer decoded_buffer;
    Ref<AudioStream> decoded_stream;

    // Ensures decoded_buffer matches `stream`, uploading it and pointing
    // buffer_handle at it if needed. No-op once already decoded for the
    // current `stream`.
    void ensure_stream_decoded();

    float gain = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool autoplay = false;

    // Hook for subclasses to configure a freshly-created ALSource before it
    // starts playing (position/relative-flag/distance-model knobs) - called
    // once per play() right after the shared gain/pitch/looping/buffer setup.
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

    // Shared muffling filter (direct/dry path) - lazily created on first
    // UpdateFilter/Play call, matching ALSource3D.cs's `filter` field.
    ALFilter filter;

    // Reverb slot this node currently sends to - not owned here (points into
    // VAWorld's listener_reverb_effect, or one of its per-zone
    // grouped_reverb_effects if this node's emitter affects grouped EAX -
    // see VAWorld::get_reverb_effect). Null means "no reverb send yet".
    ALReverbEffect *effect = nullptr;

    // Starts a new ALSource playing buffer_handle, pushes the current
    // filter/effect state to it, and appends it to `sources`. Matches
    // ALSource3D.cs's Play(): always adds a new source rather than
    // reusing/replacing an existing one. Subclass-specific positioning is
    // applied via configure_source.
    virtual bool play();

    // Stops every currently-live source (does not remove them from the list
    // - matches ALSource3D.cs's Stop()).
    void stop();

    // True once every source has finished (matches ALSource3D.cs's
    // IsPlaying(), including its "true means all finished" naming footgun -
    // kept as-is rather than fixed, per the C# reference).
    bool is_playing() const;

    // Updates (or lazily creates) the shared filter, then pushes
    // (effect, direct_filter, reverb_filter) onto every currently-live
    // source - matches ALSource3D.cs's UpdateFilter. fullReverb=true sends
    // the clean (unfiltered) signal to the reverb effect - matching
    // VASource.cs's always-fullReverb=true convention (muffling only ever
    // applies to the direct path).
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

    // Sets which OpenAL buffer new sources play - see buffer_handle above.
    // No sound-name/resource-loading registry exists yet (deferred, unlike
    // ALSource3D.cs's SoundName + ALManager.TryCreateSource); callers must
    // upload their own ALBuffer and pass its handle directly for now.
    void set_buffer_handle(ALuint value)
    {
        buffer_handle = value;
    }

    // Inspector/GDScript-facing sound assignment - the normal way to give a
    // VASource/VASourceRelative/VASourceAmbient something to play. Just
    // stores the resource; decoding is deferred to the first play() (see
    // ensure_stream_decoded).
    Ref<AudioStream> get_stream() const
    {
        return stream;
    }

    void set_stream(const Ref<AudioStream> &value)
    {
        stream = value;
    }

protected:
    // Exposed for subclasses' own _process overrides that need to touch
    // every currently-live source (e.g. ALSourceNode3D's per-frame position
    // sync).
    std::vector<std::unique_ptr<ALSource>> &get_sources()
    {
        return sources;
    }
};
