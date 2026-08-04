#pragma once

#include "al_source_node3d.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

// Port of vaudio-godot-openal's VASourceRelative.cs. Reads the listener's
// already-computed reverb effect rather than owning its own VAEmitter - a
// "relative" source is meant to sound the same regardless of where the
// listener actually is (e.g. UI/footstep-style sounds attached to the
// player), so it skips muffling (full/unfiltered gain) and only sends into
// the listener's single global reverb slot.
class VASourceRelative : public ALSourceNode3D
{
    GDCLASS(VASourceRelative, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Set while _enter_tree found no VAWorld yet - see VAEmitter's identical
    // waiting_for_world field for the rationale.
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;
    void _ready() override;

    // Matches VASourceRelative.cs's Play() override: routes into the
    // listener's reverb effect with a full (unfiltered) direct+reverb gain,
    // then defers to ALSourceNode3D::play().
    bool play() override;
};
