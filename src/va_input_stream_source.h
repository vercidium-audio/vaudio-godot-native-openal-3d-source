#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>

#include "openal/al_capture_device.h"
#include "va_stream_source.h"

using namespace godot;

// A VAStreamSource fed automatically by a local input (microphone) capture
// device, instead of a script manually calling push_audio_data(). Split out
// of what used to be VAStreamSource's own capture_device/open_capture/
// start_capture/stop_capture/close_capture members (see
// godot_stream_plan.md section 9) so the base class stays agnostic about
// where its audio comes from - VANetworkedStreamSource is the sibling for
// audio arriving over the network instead.
class VAInputStreamSource : public VAStreamSource
{
    GDCLASS(VAInputStreamSource, VAStreamSource);

private:
    // Feeds this stream directly - see open_capture(). Kept as a convenience
    // so a script doesn't need a separate node/wiring for the common "play
    // back what the mic just heard" case (godot_stream_plan.md section 6's
    // SequenceEcho.cs wiring, folded into this one node rather than left
    // fully decoupled).
    ALCaptureDevice capture_device;
    bool capture_open = false;

    // Exported settings driving the automatic open_stream()+open_capture()+
    // start_capture() sequence _ready() runs - previously these were
    // hardcoded constants in a user script (stream_test.gd's
    // AL_FORMAT_MONO16/SAMPLE_RATE), moved here so a script isn't required
    // at all for the common case of "play back this node's own microphone".
    int format = 4353; // AL_FORMAT_MONO16
    int sample_rate = 44100;
    String device_name;
    int buffer_size_frames = 1024;

protected:
    static void _bind_methods();

public:
    VAInputStreamSource();
    ~VAInputStreamSource();

    void _ready() override;
    void _process(double delta) override;

    // Turns device_name into an editor dropdown of the driver's currently
    // available capture devices - matches VAOpenALSettings::device_name's
    // identical PROPERTY_HINT_ENUM_SUGGESTION pattern (a suggestion, not a
    // strict enum, since the device list is only known at runtime and a
    // strict PROPERTY_HINT_ENUM would blank out a saved device name that
    // isn't in the list currently being queried).
    void _validate_property(PropertyInfo &p_property) const;

    int get_format() const
    {
        return format;
    }

    void set_format(int value)
    {
        format = value;
    }

    int get_sample_rate() const
    {
        return sample_rate;
    }

    void set_sample_rate(int value)
    {
        sample_rate = value;
    }

    String get_device_name() const
    {
        return device_name;
    }

    void set_device_name(const String &value)
    {
        device_name = value;
    }

    int get_buffer_size_frames() const
    {
        return buffer_size_frames;
    }

    void set_buffer_size_frames(int value)
    {
        buffer_size_frames = value;
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
    // by the destructor, which runs before ~VAStreamSource (and therefore
    // before the underlying stream_buffer is destroyed).
    void close_capture();

    bool is_capture_open() const
    {
        return capture_open;
    }
};
