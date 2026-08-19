#pragma once

#include <godot_cpp/variant/packed_byte_array.hpp>

#include "al_source_node3d.h"
#include "openal/al_capture_device.h"
#include "openal/al_stream_buffer.h"

using namespace godot;

class ALSource;

// A source that plays live, caller-pushed PCM data instead of a fixed
// AudioStream resource - godot_stream_plan.md section 5.3. The native C++
// equivalent of openal_soft_bindings's managed/ALStreamSource.cs, for use
// cases like microphone/VOIP audio where the full sound isn't known/decodable
// up front. Kept as its own class rather than folded into VASource/
// ALSourceNode (see godot_stream_plan.md section 8's "keep them separate"
// answer) since it doesn't use ALSourceNode's decode-then-upload ALBuffer
// pool at all - open_stream() creates one ALStreamBuffer + one ALSource
// directly instead of going through ALSourceNode::play()/configure_source().
//
// Extends ALSourceNode3D (not ALSourceNode directly) purely to inherit 3D
// position/max_distance/reference_distance/filter/reverb routing - the
// `streams`/`autoplay`/`looping`/decode-task machinery ALSourceNode also
// brings along is simply unused here (open_stream() bypasses play()
// entirely, so none of it ever runs).
class VAStreamSource : public ALSourceNode3D
{
    GDCLASS(VAStreamSource, ALSourceNode3D);

private:
    ALStreamBuffer stream_buffer;

    // True once open_stream() has created stream_buffer + a live ALSource -
    // guards push_audio_data/close_stream against being called out of order.
    bool stream_open = false;

    // Optional microphone input feeding this stream directly - see
    // open_capture(). Kept as a convenience so a script doesn't need a
    // separate node/wiring for the common "play back what the mic just
    // heard" case (godot_stream_plan.md section 6's SequenceEcho.cs wiring,
    // folded into this one node rather than left fully decoupled).
    ALCaptureDevice capture_device;
    bool capture_open = false;

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

    // Creates the internal ALStreamBuffer with the given OpenAL format (e.g.
    // AL_FORMAT_MONO16) and frequency, attaches it to a freshly-created
    // ALSource configured the same way ALSourceNode3D::configure_source
    // configures any other source (position/max_distance/reference_distance),
    // and starts that source playing immediately - unlike ALSourceNode::play(),
    // there's no decode step to wait on, so playback (of silence, until the
    // first push_audio_data call) begins right away. Closes any previously
    // open stream first. Returns false (with a Godot error already logged) on
    // failure - e.g. AL_SOFT_callback_buffer isn't available on this device.
    bool open_stream(int format, int frequency);

    // Copies data's bytes (raw PCM, in the format passed to open_stream - no
    // decoding happens here, see godot_stream_plan.md section 5.4) into a new
    // chunk and queues it for playback. No-ops with a logged error if
    // open_stream() hasn't been called (or close_stream() already tore the
    // stream down).
    void push_audio_data(const PackedByteArray &data);

    // Stops and destroys the underlying ALSource and ALStreamBuffer. Safe to
    // call even if open_stream() was never called or already closed.
    void close_stream();

    bool is_stream_open() const
    {
        return stream_open;
    }

    // Opens a microphone/input device (device_name empty for the driver's
    // default - see VAOpenALSettings::get_available_capture_devices for
    // valid names) and, once start_capture() is called, automatically feeds
    // every captured chunk into this same stream via push_audio_data each
    // _process - a script only needs open_stream()+open_capture()+
    // start_capture(), no manual per-frame wiring. format/frequency should
    // normally match what open_stream() was given, since no resampling
    // happens between capture and playback. Returns false (with a Godot
    // error already logged) on failure - e.g. no such capture device, or
    // ALC_EXT_capture isn't available.
    bool open_capture(const String &device_name, int format, int frequency, int buffer_size_frames);

    void start_capture();
    void stop_capture();

    // Closes the capture device (stopping it first if needed). Safe to call
    // even if open_capture() was never called or already closed. Also called
    // by close_stream() and the destructor.
    void close_capture();

    bool is_capture_open() const
    {
        return capture_open;
    }
};
