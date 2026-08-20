#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>

#include "openal/al_capture_device.h"
#include "va_device_name.h"
#include "va_stream_source.h"

using namespace godot;

// A VAStreamSource fed automatically by a local input (microphone) capture
// device, instead of a script manually calling push_audio_data(). Split out
// of what used to be VAStreamSource's own capture_device/open_capture/
// start_capture/stop_capture/close_capture members (see
// godot_stream_plan.md section 9) so the base class stays agnostic about
// where its audio comes from - VANetworkedStreamSource is the sibling for
// audio arriving over the network instead.
//
// Besides feeding its own playback (the "hear your own mic" case), this node
// also emits the "audio_captured" signal with every chunk it captures - see
// todo.md's "Godot" section ("need a way to give the user the [microphone]
// data stream ... so they can send it over VOIP"). A script connects to that
// signal and forwards the PackedByteArray over the network itself (ENet/RPC/
// PacketPeerUDP etc. are scripting-only, same reasoning as
// VANetworkedStreamSource not wrapping them) - this node has no opinion on
// how the data is transmitted, only on capturing it and handing it out.
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

    // Minimum peak sample amplitude (0-32767, matching a 16-bit PCM sample's
    // range regardless of the capture format actually in use - 8-bit samples
    // are scaled up before comparing) a captured chunk must reach before it's
    // forwarded to push_audio_data() - see todo.md's "Godot" section:
    // "microphone threshold should be a setting on the node that accepts
    // microphone input from the user's capture device". 0 (the default)
    // forwards every chunk, matching the C# reference's MicrophoneThreshold
    // defaulting to 0 (ALManager.cs, vaudio-godot-mono-openal-3d) before this
    // was wired up to anything there.
    int microphone_threshold = 0;

    // Returns the greatest absolute sample amplitude in a chunk of format-
    // encoded PCM data, scaled to a 16-bit range so microphone_threshold
    // means the same thing regardless of format. Used by the open_capture()
    // data callback to gate push_audio_data() and the "audio_captured"
    // signal on microphone_threshold.
    static int peak_amplitude(const uint8_t *data, int num_bytes, int format);

protected:
    static void _bind_methods();

public:
    VAInputStreamSource();
    ~VAInputStreamSource();

    void _ready() override;
    void _process(double delta) override;

    // Turns device_name into an editor dropdown of the driver's currently
    // available capture devices.
    void _validate_property(PropertyInfo &p_property) const;

    // Re-queries the driver's capture device list and re-validates
    // device_name's PROPERTY_HINT_ENUM so a device plugged in after this
    // node was selected shows up without reselecting the node - see
    // VADeviceRefreshInspectorPlugin (va_conversion_plugin.h) for the
    // Inspector button that calls this.
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

    // device_name itself stores "" for "use the driver's default capture
    // device" (what open_capture expects - see _ready()'s direct use of the
    // field below), but the getter/setter exposed to the inspector translate
    // that to/from DEFAULT_DEVICE_LABEL, since the Inspector's enum dropdown
    // (_validate_property) shows the property's current value as one of its
    // entries and an unlabelled "" would otherwise show as a blank entry.
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

    // Opens a microphone/input device (device_name empty for the driver's
    // default - see ALManager::get_available_capture_devices for valid
    // names) and, once start_capture() is called, automatically feeds
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
