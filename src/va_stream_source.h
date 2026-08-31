#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "openal/al_stream_buffer.h"
#include "va_raytraced_source.h"

using namespace godot;

class ALSourceHandle;

class VAStreamSource : public VARaytracedSource
{
    GDCLASS(VAStreamSource, VARaytracedSource);

private:
    ALStreamBuffer stream_buffer;

    bool stream_open = false;

    std::vector<uint8_t> drained_chunk;

    void drain_used_chunks();

protected:
    static void _bind_methods();

public:
    VAStreamSource();
    ~VAStreamSource();

    void _process(double delta) override;

    void _validate_property(PropertyInfo &p_property) const;

    bool open_stream(int format, int frequency);

    void push_audio_data(const PackedByteArray &data);

    void push_audio_data(const uint8_t *data, int num_bytes);

    void close_stream();

    bool is_stream_open() const
    {
        return stream_open;
    }
};
