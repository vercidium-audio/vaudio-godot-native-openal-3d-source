#pragma once

#include "va_stream_source.h"

using namespace godot;

class VANetworkedStreamSource : public VAStreamSource
{
    GDCLASS(VANetworkedStreamSource, VAStreamSource);

protected:
    // Empty - declared explicitly (rather than relying on fallback to VAStreamSource::_bind_methods) to match this repo's convention of every GDCLASS having its own.
    static void _bind_methods()
    {
    }

public:
    VANetworkedStreamSource();
    ~VANetworkedStreamSource();
};
