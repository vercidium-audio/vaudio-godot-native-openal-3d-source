#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>

#include "openal/al_capture_device.h"
#include "va_device_name.h"
#include "va_stream_source.h"

using namespace godot;

class VAInputStreamSource : public VAStreamSource
{
    GDCLASS(VAInputStreamSource, VAStreamSource);

private:
    // Feeds this stream directly - see open_capture(). Kept as a convenience for the common "play back what the mic just heard" case.
    ALCaptureDevice capture_device;
    bool capture_open = false;

    // Exported settings driving the automatic open_stream()+open_capture()+start_capture() sequence _ready() runs.
    int format = 4353; // AL_FORMAT_MONO16
    int sample_rate = 44100;
    String device_name;
    int buffer_size_frames = 1024;

    // Minimum peak sample amplitude (0-32767, a 16-bit PCM range regardless of capture format) a chunk must reach before being forwarded to push_audio_data(). 0 forwards every chunk.
    int microphone_threshold = 0;

    // Greatest absolute sample amplitude in a chunk of format-encoded PCM data, scaled to a 16-bit range so microphone_threshold means the same thing regardless of format.
    static int peak_amplitude(const uint8_t *data, int num_bytes, int format);

protected:
    static void _bind_methods();

public:
    VAInputStreamSource();
    ~VAInputStreamSource();

    void _ready() override;
    void _process(double delta) override;

    // Turns device_name into an editor dropdown of the driver's currently available capture devices.
    void _validate_property(PropertyInfo &p_property) const;

    // Re-queries the driver's capture device list so a device plugged in after this node was selected shows up - see VADeviceRefreshInspectorPlugin's Inspector button that calls this.
    void refresh_devices();

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

    // device_name stores "" for "use the driver's default", but the inspector getter/setter translate that to/from DEFAULT_DEVICE_LABEL so the enum dropdown doesn't show a blank entry.
    String get_device_name() const
    {
        return device_name.is_empty() ? DEFAULT_DEVICE_LABEL : device_name;
    }

    void set_device_name(const String &value);

    int get_buffer_size_frames() const
    {
        return buffer_size_frames;
    }

    void set_buffer_size_frames(int value)
    {
        buffer_size_frames = value;
    }

    int get_microphone_threshold() const
    {
        return microphone_threshold;
    }

    void set_microphone_threshold(int value)
    {
        microphone_threshold = value < 0 ? 0 : value;
    }

    bool open_capture(const String &device_name, int format, int frequency, int buffer_size_frames);

    void start_capture();
    void stop_capture();

    // Safe to call even if open_capture() was never called or already closed. Also called by the destructor, before ~VAStreamSource destroys the underlying stream_buffer.
    void close_capture();

    bool is_capture_open() const
    {
        return capture_open;
    }
};
