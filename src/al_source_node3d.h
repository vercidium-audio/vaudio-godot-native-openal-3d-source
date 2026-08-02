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

// Native C++ equivalent of godot-openal's nodes/ALSource3D.cs, the base
// VASource extends (native_godot_plan.md "Implement VASource's per-frame
// result application"). Owns a list of concurrently-live ALSource instances
// (not just one) so overlapping one-shot plays don't cut each other off,
// matching ALSource3D.cs's own `sources` list - a Play() call always starts a
// new ALSource and appends it, never replaces an existing one.
class ALSourceNode3D : public Node3D
{
    GDCLASS(ALSourceNode3D, Node3D);

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
    bool relative = false;

    // AL_INVERSE_DISTANCE_CLAMPED's clamp bounds (see ALManager::initialize).
    // max_distance=0 is a degenerate clamp (distance is clamped to
    // [reference_distance, 0], i.e. never allowed past reference_distance),
    // so this needs a real default rather than OpenAL's own alSourcef
    // default of 0 - 100 world units is a reasonable "basically inaudible
    // past here" default for a 3D game scene.
    float max_distance = 100.0f;
    float reference_distance = 1.0f;

public:
    ALSourceNode3D();
    ~ALSourceNode3D();

    void _process(double delta) override;

    // Shared muffling filter (direct/dry path) - lazily created on first
    // UpdateFilter/Play call, matching ALSource3D.cs's `filter` field.
    ALFilter filter;

    // Reverb slot this node currently sends to - not owned here (points into
    // VAWorld's listener_reverb_effect, or one of its per-zone
    // grouped_reverb_effects if this node's emitter affects grouped EAX -
    // see VAWorld::get_reverb_effect). Null means "no reverb send yet".
    ALReverbEffect *effect = nullptr;

    // Starts a new ALSource playing buffer_handle at this node's current
    // global position, pushes the current filter/effect state to it, and
    // appends it to `sources`. Matches ALSource3D.cs's Play(): always adds a
    // new source rather than reusing/replacing an existing one.
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
    void set_relative(bool value);
    void set_max_distance(float value);
    void set_reference_distance(float value);

    // Inspector-facing accessors for every ALSourceNode3D tuning knob (see
    // the field block above). Note VASourceRelative::_ready() force-sets
    // `relative` to true on itself right after entering the tree (that's the
    // whole point of that subclass), so editing this property on a
    // VASourceRelative node has no lasting effect - only meaningful on plain
    // VASource/VASourceAmbient nodes.
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

    bool get_relative() const
    {
        return relative;
    }

    float get_max_distance() const
    {
        return max_distance;
    }

    float get_reference_distance() const
    {
        return reference_distance;
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
};
