#include "va_input_stream_source.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "openal/al_manager.h"
#include "va_engine_util.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

VAInputStreamSource::VAInputStreamSource()
{
}

VAInputStreamSource::~VAInputStreamSource()
{
    close_capture();
}

void VAInputStreamSource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_format"), &VAInputStreamSource::get_format);
    ClassDB::bind_method(D_METHOD("set_format", "value"), &VAInputStreamSource::set_format);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "format", PROPERTY_HINT_ENUM, "Mono 8-bit:4352,Mono 16-bit:4353,Stereo 8-bit:4354,Stereo 16-bit:4355"), "set_format", "get_format");

    ClassDB::bind_method(D_METHOD("get_sample_rate"), &VAInputStreamSource::get_sample_rate);
    ClassDB::bind_method(D_METHOD("set_sample_rate", "value"), &VAInputStreamSource::set_sample_rate);
    // Unlike ALManager's playback device, ALCaptureDevice::open() has no "use the driver's default rate" concept, so there's no "System Default" entry here.
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_rate", PROPERTY_HINT_ENUM, "22050,44100,48000,96000"), "set_sample_rate", "get_sample_rate");

    ClassDB::bind_method(D_METHOD("get_device_name"), &VAInputStreamSource::get_device_name);
    ClassDB::bind_method(D_METHOD("set_device_name", "value"), &VAInputStreamSource::set_device_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_name", PROPERTY_HINT_ENUM, DEFAULT_DEVICE_LABEL), "set_device_name", "get_device_name");

    ClassDB::bind_method(D_METHOD("get_buffer_size_frames"), &VAInputStreamSource::get_buffer_size_frames);
    ClassDB::bind_method(D_METHOD("set_buffer_size_frames", "value"), &VAInputStreamSource::set_buffer_size_frames);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "buffer_size_frames", PROPERTY_HINT_RANGE, "1,65536,1,or_greater"), "set_buffer_size_frames", "get_buffer_size_frames");

    ClassDB::bind_method(D_METHOD("get_microphone_threshold"), &VAInputStreamSource::get_microphone_threshold);
    ClassDB::bind_method(D_METHOD("set_microphone_threshold", "value"), &VAInputStreamSource::set_microphone_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "microphone_threshold", PROPERTY_HINT_RANGE, "0,32767,or_greater"), "set_microphone_threshold", "get_microphone_threshold");

    ClassDB::bind_method(D_METHOD("open_capture", "device_name", "format", "frequency", "buffer_size_frames"), &VAInputStreamSource::open_capture);

    ClassDB::bind_method(D_METHOD("refresh_devices"), &VAInputStreamSource::refresh_devices);
    ClassDB::bind_method(D_METHOD("start_capture"), &VAInputStreamSource::start_capture);
    ClassDB::bind_method(D_METHOD("stop_capture"), &VAInputStreamSource::stop_capture);
    ClassDB::bind_method(D_METHOD("close_capture"), &VAInputStreamSource::close_capture);
    ClassDB::bind_method(D_METHOD("is_capture_open"), &VAInputStreamSource::is_capture_open);

    // Lets a script get at the raw microphone data (e.g. to send it over VOIP) without having to
    // duplicate this node's own capture device
    ADD_SIGNAL(MethodInfo("audio_captured", PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data")));
}

// Uses PROPERTY_HINT_ENUM rather than PROPERTY_HINT_ENUM_SUGGESTION since Godot's editor unconditionally injects a blank entry into the latter for Variant::STRING, with no way to suppress it.
void VAInputStreamSource::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name != StringName("device_name"))
        return;

    ALManager *manager = ALManager::get_singleton();

    if (!manager)
        return;

    PackedStringArray devices = manager->get_available_capture_devices();

    if (devices.is_empty())
        return;

    devices.insert(0, DEFAULT_DEVICE_LABEL);

    p_property.hint = PROPERTY_HINT_ENUM;
    p_property.hint_string = String(",").join(devices);
}

void VAInputStreamSource::set_device_name(const String &value)
{
    device_name = value == DEFAULT_DEVICE_LABEL ? String() : value;
}

void VAInputStreamSource::refresh_devices()
{
    notify_property_list_changed();
}

// Matches ALSource::_ready()'s autoplay pattern: runs the open_stream()/open_capture()/start_capture() sequence once, driven by this node's own exported properties instead of a user script.
void VAInputStreamSource::_ready()
{
    VAStreamSource::_ready();

    if (IS_EDITOR_HINT())
    {
        return;
    }

    if (!open_stream(format, sample_rate))
    {
        return;
    }

    if (!open_capture(device_name, format, sample_rate, buffer_size_frames))
    {
        return;
    }

    start_capture();
}

// AL_FORMAT_MONO8/STEREO8 samples are unsigned (silence = 128); MONO16/STEREO16 are signed (silence = 0) - 8-bit amplitudes are scaled up by 256 so microphone_threshold means the same thing regardless of format.
int VAInputStreamSource::peak_amplitude(const uint8_t *data, int num_bytes, int format)
{
    int peak = 0;

    if (format == AL_FORMAT_MONO8 || format == AL_FORMAT_STEREO8)
    {
        for (int i = 0; i < num_bytes; i++)
        {
            int amplitude = abs((int)data[i] - 128) * 256;
            peak = amplitude > peak ? amplitude : peak;
        }
    }
    else
    {
        const int16_t *samples = reinterpret_cast<const int16_t *>(data);
        int sample_count = num_bytes / 2;

        for (int i = 0; i < sample_count; i++)
        {
            int amplitude = abs((int)samples[i]);
            peak = amplitude > peak ? amplitude : peak;
        }
    }

    return peak;
}

bool VAInputStreamSource::open_capture(const String &device_name, int format, int frequency, int buffer_size_frames)
{
    close_capture();

    // Captured audio is dropped (not errored) until a stream is opened, so open_capture()/start_capture() may legitimately
    // run before open_stream(). "audio_captured" is emitted independently of is_stream_open() so a script can use this node
    // purely to get at the microphone (e.g. for VOIP), gated on microphone_threshold the same as local playback.
    bool opened = capture_device.open(device_name, (ALenum)format, frequency, buffer_size_frames,
        [this, format](const uint8_t *data, int num_bytes)
        {
            if (peak_amplitude(data, num_bytes, format) < microphone_threshold)
            {
                return;
            }

            if (is_stream_open())
            {
                push_audio_data(data, num_bytes);
            }

            PackedByteArray packed;
            packed.resize(num_bytes);
            memcpy(packed.ptrw(), data, num_bytes);
            emit_signal("audio_captured", packed);
        });

    if (!opened)
    {
        // ALCaptureDevice::open already logged the reason for failure.
        return false;
    }

    capture_open = true;
    return true;
}

void VAInputStreamSource::start_capture()
{
    if (!capture_open)
    {
        VA_ERROR_NAMED("start_capture called before open_capture (or after close_capture)");
        return;
    }

    capture_device.start();
}

void VAInputStreamSource::stop_capture()
{
    if (!capture_open)
    {
        return;
    }

    capture_device.stop();
}

void VAInputStreamSource::close_capture()
{
    if (!capture_open)
    {
        return;
    }

    capture_device.close();
    capture_open = false;
}

void VAInputStreamSource::_process(double delta)
{
    VAStreamSource::_process(delta);

    if (capture_open)
    {
        capture_device.update();
    }
}
