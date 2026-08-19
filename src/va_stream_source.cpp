#include "va_stream_source.h"

#include <godot_cpp/core/class_db.hpp>

#include "openal/al_source.h"
#include "va_engine_util.h"

VAStreamSource::VAStreamSource()
{
}

VAStreamSource::~VAStreamSource()
{
    close_capture();
    close_stream();
}

void VAStreamSource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open_stream", "format", "frequency"), &VAStreamSource::open_stream);
    ClassDB::bind_method(D_METHOD("push_audio_data", "data"), &VAStreamSource::push_audio_data);
    ClassDB::bind_method(D_METHOD("close_stream"), &VAStreamSource::close_stream);
    ClassDB::bind_method(D_METHOD("is_stream_open"), &VAStreamSource::is_stream_open);

    ClassDB::bind_method(D_METHOD("open_capture", "device_name", "format", "frequency", "buffer_size_frames"), &VAStreamSource::open_capture);
    ClassDB::bind_method(D_METHOD("start_capture"), &VAStreamSource::start_capture);
    ClassDB::bind_method(D_METHOD("stop_capture"), &VAStreamSource::stop_capture);
    ClassDB::bind_method(D_METHOD("close_capture"), &VAStreamSource::close_capture);
    ClassDB::bind_method(D_METHOD("is_capture_open"), &VAStreamSource::is_capture_open);
}

bool VAStreamSource::open_stream(int format, int frequency)
{
    close_stream();

    if (!stream_buffer.create((ALenum)format, frequency))
    {
        // ALStreamBuffer::create already logged the reason for failure.
        return false;
    }

    auto source = std::make_unique<ALSource>();

    if (!source->create())
    {
        // ALSource::create already logged the reason for failure.
        stream_buffer.destroy();
        return false;
    }

    source->set_buffer(stream_buffer.get_handle());
    source->set_gain(get_gain());
    source->set_pitch(get_pitch());

    configure_source(*source);

    source->play();

    get_sources().push_back(std::move(source));
    stream_open = true;
    return true;
}

void VAStreamSource::push_audio_data(const PackedByteArray &data)
{
    if (!stream_open)
    {
        VA_ERROR_NAMED("push_audio_data called before open_stream (or after close_stream)");
        return;
    }

    if (data.size() == 0)
    {
        return;
    }

    stream_buffer.enqueue(data.ptr(), (int)data.size());
}

void VAStreamSource::close_stream()
{
    if (!stream_open)
    {
        return;
    }

    stop();
    stream_buffer.destroy();
    stream_open = false;
}

bool VAStreamSource::open_capture(const String &device_name, int format, int frequency, int buffer_size_frames)
{
    close_capture();

    // Bytes are pushed straight into stream_buffer, not through
    // push_audio_data()'s stream_open check - a script may legitimately call
    // open_capture()/start_capture() before open_stream(), in which case
    // captured audio is simply dropped until a stream is opened, rather than
    // erroring every frame like push_audio_data() would.
    bool opened = capture_device.open(device_name, (ALenum)format, frequency, buffer_size_frames,
        [this](const uint8_t *data, int num_bytes)
        {
            if (stream_open)
            {
                stream_buffer.enqueue(data, num_bytes);
            }
        });

    if (!opened)
    {
        // ALCaptureDevice::open already logged the reason for failure.
        return false;
    }

    capture_open = true;
    return true;
}

void VAStreamSource::start_capture()
{
    if (!capture_open)
    {
        VA_ERROR_NAMED("start_capture called before open_capture (or after close_capture)");
        return;
    }

    capture_device.start();
}

void VAStreamSource::stop_capture()
{
    if (!capture_open)
    {
        return;
    }

    capture_device.stop();
}

void VAStreamSource::close_capture()
{
    if (!capture_open)
    {
        return;
    }

    capture_device.close();
    capture_open = false;
}

void VAStreamSource::_process(double delta)
{
    ALSourceNode3D::_process(delta);

    if (capture_open)
    {
        capture_device.update();
    }

    drain_used_chunks();
}

void VAStreamSource::drain_used_chunks()
{
    // Just discards each used chunk's bytes - VAStreamSource owns no
    // long-lived copy of pushed data to free beyond drained_chunk itself
    // (unlike ALStreamSource.cs's TryGetUsedData, which hands the chunk back
    // to the caller to free/recycle; there's no equivalent recycling need on
    // the GDScript side of push_audio_data's PackedByteArray copy).
    while (stream_buffer.try_get_used_chunk(drained_chunk))
    {
    }
}
