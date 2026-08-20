#pragma once

#include "va_stream_source.h"

using namespace godot;

// A VAStreamSource intended to be fed by network data instead of a local input device - see VAInputStreamSource for the
// microphone equivalent. Godot's networking APIs are entirely scripting-layer with nothing here to wrap, so this adds no
// methods beyond VAStreamSource; it exists purely as a distinctly-named node type. A script's multiplayer packet handler
// should call push_audio_data(data) directly on this node each time a voice chunk arrives over the network.
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
