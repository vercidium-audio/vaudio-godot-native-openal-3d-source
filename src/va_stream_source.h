#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "openal/al_stream_buffer.h"
#include "va_raytraced_source.h"

using namespace godot;

class ALSourceHandle;

// A source that plays live, caller-pushed PCM data instead of a fixed AudioStream resource - the native equivalent of
// openal_soft_bindings's ALStreamSource.cs, for cases like microphone/VOIP audio where the sound isn't decodable up front.
// Kept separate from VASource/ALSource since it bypasses ALSource's decode-then-upload buffer pool entirely: open_stream()
// creates one ALStreamBuffer + ALSourceHandle directly instead of going through ALSource::play()/configure_source().
// Extends VARaytracedSource (not ALSource3D) to get the same raytraced reverb/muffling/ambience behaviour as VASource, since
// streaming vs. fixed-buffer playback is purely an AL-layer difference. This base class only accepts already-decoded PCM
// bytes; see VAInputStreamSource (microphone) and VANetworkedStreamSource (network packets from script) for concrete uses.
class VAStreamSource : public VARaytracedSource
{
    GDCLASS(VAStreamSource, VARaytracedSource);

private:
    ALStreamBuffer stream_buffer;

    // True once open_stream() has created stream_buffer + a live ALSourceHandle; guards push_audio_data/close_stream against being called out of order.
    bool stream_open = false;

    // Reused across try_get_used_chunk() calls so drain_used_chunks() doesn't allocate a new std::vector for every used chunk.
    std::vector<uint8_t> drained_chunk;

    void drain_used_chunks();

protected:
    static void _bind_methods();

public:
    VAStreamSource();
    ~VAStreamSource();

    void _process(double delta) override;

    // Hides ALSource's fixed-buffer-only inspector properties (streams, looping, autoplay, pitch_randomness,
    // volume_randomness_db, playback_no_repeat) since none apply to a stream source (see the class comment above).
    void _validate_property(PropertyInfo &p_property) const;

    // Creates the internal ALStreamBuffer with the given OpenAL format/frequency, attaches it to a freshly-created
    // ALSourceHandle, and starts it playing immediately - unlike ALSource::play() there's no decode step to wait on, so
    // playback (of silence until the first push_audio_data call) begins right away. Closes any previously open stream first.
    bool open_stream(int format, int frequency);

    // Copies data's raw PCM bytes into a new chunk and queues it for playback; no-ops with a logged error if open_stream()
    // hasn't been called. Single entry point every audio source feeds through - a microphone callback (VAInputStreamSource)
    // or a received network packet (VANetworkedStreamSource, from a script's multiplayer/RPC handler) both call this.
    void push_audio_data(const PackedByteArray &data);

    // Raw-pointer overload for C++-internal callers only (not exposed to GDScript) - avoids the PackedByteArray copy for a hot per-frame path like VAInputStreamSource's capture callback.
    void push_audio_data(const uint8_t *data, int num_bytes);

    // Stops and destroys the underlying ALSourceHandle and ALStreamBuffer. Safe to call even if open_stream() was never called or already closed.
    void close_stream();

    bool is_stream_open() const
    {
        return stream_open;
    }
};
