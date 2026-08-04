#pragma once

#include "al_source_node.h"

using namespace godot;

class ALSource;

// Non-positional ALSourceNode - for sounds that should be heard the same
// regardless of where this node or the listener are in the world (UI/
// footstep-style one-shots via VASourceRelative, background ambience loops
// via VASourceAmbient). Every source this creates is AL_SOURCE_RELATIVE with
// its AL_POSITION pinned to the origin, i.e. exactly at the listener - OpenAL
// still treats AL_SOURCE_RELATIVE sources as positional/panned, just in
// listener-local space rather than world space, so leaving position at
// anything other than zero (e.g. this node's own world position, as the old
// combined ALSourceNode3D did via a `relative` bool) makes the sound pan
// toward wherever that offset points instead of surrounding the listener.
class ALSourceNodeRelative : public ALSourceNode
{
    GDCLASS(ALSourceNodeRelative, ALSourceNode);

protected:
    static void _bind_methods();

    // ALSourceNode::play() hook: marks the freshly-created source
    // AL_SOURCE_RELATIVE with a zero position, so it's heard the same
    // regardless of this node's or the listener's world position.
    void configure_source(ALSource &source) override;

public:
    ALSourceNodeRelative();
    ~ALSourceNodeRelative();
};
