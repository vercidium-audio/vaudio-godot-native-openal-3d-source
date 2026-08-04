#pragma once

#include "al_source_node_relative.h"

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
// the listener's single global reverb slot. Extends ALSourceNodeRelative (not
// ALSourceNode3D) so its sources are always AL_SOURCE_RELATIVE with a pinned
// origin position - see that class's doc comment for why this used to be a
// `relative` bool on ALSourceNode3D and caused mispositioned/panned audio.
class VASourceRelative : public ALSourceNodeRelative
{
    GDCLASS(VASourceRelative, ALSourceNodeRelative);

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

    // Matches VASourceRelative.cs's Play() override: routes into the
    // listener's reverb effect with a full (unfiltered) direct+reverb gain,
    // then defers to ALSourceNodeRelative::play().
    bool play() override;
};
