#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "openal/al_stream_buffer.h"
#include "va_raytraced_source.h"

using namespace godot;

class ALSourceHandle;

// A source that plays live, caller-pushed PCM data instead of a fixed
// AudioStream resource. The native C++
// equivalent of openal_soft_bindings's managed/ALStreamSource.cs, for use
// cases like microphone/VOIP audio where the full sound isn't known/decodable
// up front. Kept as its own class rather than folded into VASource/
// ALSource since it doesn't use ALSource's decode-then-upload ALBuffer
// pool at all - open_stream() creates one ALStreamBuffer + one ALSourceHandle
// directly instead of going through ALSource::play()/configure_source().
//
// Extends VARaytracedSource (not ALSource3D directly) to get the
// same raytraced reverb/muffling/ambience behaviour VASource has, on top of
// 3D position/max_distance/reference_distance/filter/reverb routing - per
// user, streaming vs. fixed-buffer playback is purely an AL-layer difference,
// the VA/raytracing side should be identical between VASource and
// VAStreamSource. The `streams`/`autoplay`/`looping`/decode-task machinery
// ALSource also brings along is simply unused here (open_stream()
// bypasses play() entirely, so none of it ever runs).
//
// This base class only knows how to accept already-decoded PCM bytes from
// wherever they come from - it has no opinion on the source of that data.
// See VAInputStreamSource (microphone, via ALCaptureDevice) and
// VANetworkedStreamSource (a network packet handler in a script calling
// push_audio_data directly - Godot's multiplayer/RPC/ENet/UDP APIs are
// scripting-only, so there is no C++ binding surface for this class to add)
// for the two concrete node types a scene actually uses.
class VAStreamSource : public VARaytracedSource
{
    GDCLASS(VAStreamSource, VARaytracedSource);

private:
    ALStreamBuffer stream_buffer;

    // True once open_stream() has created stream_buffer + a live ALSourceHandle -
    // guards push_audio_data/close_stream against being called out of order.
    bool stream_open = false;

    // Reused across try_get_used_chunk() calls in _process() so drain_chunks()
    // doesn't allocate a new std::vector for every used chunk.
    std::vector<uint8_t> drained_chunk;

    void drain_used_chunks();

protected:
    static void _bind_methods();

public:
    VAStreamSource();
    ~VAStreamSource();

    void _process(double delta) override;

    // Hides ALSource's fixed-buffer-only inspector properties (streams,
    // looping, autoplay, pitch_randomness, volume_randomness_db,
    // playback_no_repeat) - none of them apply to a stream source, which
    // never goes through ALSource::play()/streams/decode-task machinery
    // at all (see the class comment above). Inherited by VAInputStreamSource
    // and VANetworkedStreamSource.
    void _validate_property(PropertyInfo &p_property) const;

    // Creates the internal ALStreamBuffer with the given OpenAL format (e.g.
    // AL_FORMAT_MONO16) and frequency, attaches it to a freshly-created
    // ALSourceHandle configured the same way ALSource3D::configure_source
    // configures any other source (position/max_distance/reference_distance),
    // and starts that source playing immediately - unlike ALSource::play(),
    // there's no decode step to wait on, so playback (of silence, until the
    // first push_audio_data call) begins right away. Closes any previously
    // open stream first. Returns false (with a Godot error already logged) on
    // failure - e.g. AL_SOFT_callback_buffer isn't available on this device.
    bool open_stream(int format, int frequency);

    // Copies data's bytes (raw PCM, in the format passed to open_stream - no
    // decoding happens here) into a new
    // chunk and queues it for playback. No-ops with a logged error if
    // open_stream() hasn't been called (or close_stream() already tore the
    // stream down). This is the single entry point every audio source feeds
    // through - a microphone callback (VAInputStreamSource) or a received
    // network packet (VANetworkedStreamSource, called directly from a
    // script's multiplayer/RPC handler) both end up calling this the same
    // way.
    void push_audio_data(const PackedByteArray &data);

    // Raw-pointer overload for C++-internal callers only (not exposed to
    // GDScript) - avoids the PackedByteArray copy push_audio_data's Variant
    // signature otherwise forces, for a hot per-frame path like
    // VAInputStreamSource's capture-device callback. Same semantics as
    // push_audio_data(PackedByteArray) otherwise.
    void push_audio_data(const uint8_t *data, int num_bytes);

    // Stops and destroys the underlying ALSourceHandle and ALStreamBuffer. Safe to
    // call even if open_stream() was never called or already closed.
    void close_stream();

    bool is_stream_open() const
    {
        return stream_open;
    }
};
