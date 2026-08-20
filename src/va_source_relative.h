#pragma once

#include "al_source_relative.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

// Port of VASourceRelative.cs. Reads the listener's already-computed reverb effect rather than owning its own VAEmitter - a
// "relative" source should sound the same regardless of listener position (e.g. UI/footstep sounds on the player), so it skips
// muffling and only sends into the listener's single global reverb slot. Extends ALSourceRelative (not ALSource3D) so its
// sources stay AL_SOURCE_RELATIVE with a pinned origin (see that class's comment for why a `relative` bool on ALSource3D broke).
class VASourceRelative : public ALSourceRelative
{
    GDCLASS(VASourceRelative, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Set while _enter_tree found no VAWorld yet (see VAEmitter's identical waiting_for_world field).
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;

    // Matches VASourceRelative.cs's Play() override: routes into the listener's reverb effect with full unfiltered gain, then defers to ALSourceRelative::play().
    bool play() override;
};
