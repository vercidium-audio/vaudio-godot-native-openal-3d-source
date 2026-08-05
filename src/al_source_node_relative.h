#pragma once

#include "al_source_node.h"

using namespace godot;

class ALSource;

// Sounds that are relative to the listener, e.g. footsteps, ambience, music
class ALSourceNodeRelative : public ALSourceNode
{
    GDCLASS(ALSourceNodeRelative, ALSourceNode);

protected:
    static void _bind_methods();

    void configure_source(ALSource &source) override;

public:
    ALSourceNodeRelative();
    ~ALSourceNodeRelative();
};
