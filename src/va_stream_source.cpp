#include "va_stream_source.h"

#include <godot_cpp/core/class_db.hpp>

#include "openal/al_source_handle.h"
#include "va_engine_util.h"

VAStreamSource::VAStreamSource()
{
}

VAStreamSource::~VAStreamSource()
{
    close_stream();
}

void VAStreamSource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open_stream", "format", "frequency"), &VAStreamSource::open_stream);
    ClassDB::bind_method(D_METHOD("push_audio_data", "data"), static_cast<void (VAStreamSource::*)(const PackedByteArray &)>(&VAStreamSource::push_audio_data));
    ClassDB::bind_method(D_METHOD("close_stream"), &VAStreamSource::close_stream);
    ClassDB::bind_method(D_METHOD("is_stream_open"), &VAStreamSource::is_stream_open);
}

bool VAStreamSource::open_stream(int format, int frequency)
{
    close_stream();

    if (!stream_buffer.create((ALenum)format, frequency))
    {
        // ALStreamBuffer::create already logged the reason for failure.
        return false;
    }

    auto source = std::make_unique<ALSourceHandle>();

    if (!source->create())
    {
        // ALSourceHandle::create already logged the reason for failure.
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
    push_audio_data(data.ptr(), (int)data.size());
}

void VAStreamSource::push_audio_data(const uint8_t *data, int num_bytes)
{
    if (!stream_open)
    {
        VA_ERROR_NAMED("push_audio_data called before open_stream (or after close_stream)");
        return;
    }

    if (num_bytes == 0)
    {
        return;
    }

    stream_buffer.enqueue(data, num_bytes);
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

void VAStreamSource::_validate_property(PropertyInfo &p_property) const
{
    static const StringName hidden_properties[] = {
        "streams",
        "looping",
        "autoplay",
        "pitch_randomness",
        "volume_randomness_db",
        "playback_no_repeat",
    };

    for (const StringName &hidden_property : hidden_properties)
    {
        if (p_property.name == hidden_property)
        {
            p_property.usage = PROPERTY_USAGE_NONE;
            return;
        }
    }
}

void VAStreamSource::_process(double delta)
{
    VARaytracedSource::_process(delta);
    process_raytracing(delta);

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
