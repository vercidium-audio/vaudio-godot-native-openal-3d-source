#pragma once

#include "al_source_node3d.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

// Port of vaudio-godot-openal's VASourceAmbient.cs. Reads the listener's
// already-computed ambient (non-directional) muffling gain rather than
// owning its own VAEmitter - meant for background ambience loops that should
// be muffled by the room the listener is currently in, without being
// raytraced/positioned relative to any specific source location. Waits for
// the listener's first ambient filter result before playing, then re-applies
// the (possibly-changing) ambient gain every frame.
class VASourceAmbient : public ALSourceNode3D
{
    GDCLASS(VASourceAmbient, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;
    bool played = false;

protected:
    static void _bind_methods();

public:
    VASourceAmbient();
    ~VASourceAmbient();

    void _enter_tree() override;
    void _ready() override;
    void _process(double delta) override;

    // Matches VASourceAmbient.cs's Play() override: refuses to start until
    // the listener has produced an ambient filter result at least once.
    bool play() override;
};
