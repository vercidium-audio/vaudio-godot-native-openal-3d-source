#pragma once

#include "al_source.h"

using namespace godot;

class ALSourceHandle;

// Sounds that are relative to the listener, e.g. footsteps, ambience, music
class ALSourceRelative : public ALSource
{
    GDCLASS(ALSourceRelative, ALSource);

protected:
    static void _bind_methods();

    void configure_source(ALSourceHandle &source) override;

public:
    ALSourceRelative();
    ~ALSourceRelative();
};
