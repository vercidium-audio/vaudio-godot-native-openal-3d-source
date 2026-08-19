#pragma once

#include "va_stream_source.h"

using namespace godot;

// A VAStreamSource intended to be fed by network data instead of a local
// input device - see VAInputStreamSource for the microphone equivalent.
// Godot's multiplayer/RPC/ENet/UDP networking APIs are entirely
// scripting-layer (MultiplayerAPI, high-level multiplayer RPCs,
// PacketPeerUDP, etc.) with no native C++ binding surface this class needs
// to wrap, so this adds no methods beyond what VAStreamSource already
// exposes - it exists purely as a distinctly-named/documented node type
// (godot_stream_plan.md section 9), so a scene tree reads as "this source is
// fed by network code" at a glance, and so its own doc_classes XML can spell
// out the intended wiring. A script's multiplayer packet handler (e.g. an
// @rpc-annotated method, or a PacketPeerUDP read loop) should call
// push_audio_data(data) directly on this node each time a voice chunk
// arrives over the network.
class VANetworkedStreamSource : public VAStreamSource
{
    GDCLASS(VANetworkedStreamSource, VAStreamSource);

protected:
    // Empty - no methods/properties beyond what VAStreamSource already
    // binds. Still declared explicitly (rather than relying on C++ name
    // lookup falling back to VAStreamSource::_bind_methods) to match this
    // repo's convention of every GDCLASS having its own _bind_methods.
    static void _bind_methods()
    {
    }

public:
    VANetworkedStreamSource();
    ~VANetworkedStreamSource();
};
