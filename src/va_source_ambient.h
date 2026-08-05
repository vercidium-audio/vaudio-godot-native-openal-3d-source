#pragma once

#include "al_source_node_relative.h"

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
// the (possibly-changing) ambient gain every frame. Extends
// ALSourceNodeRelative (not ALSourceNode3D) so its sources are always
// AL_SOURCE_RELATIVE with a pinned origin position - see that class's doc
// comment for why this used to be a `relative` bool on ALSourceNode3D and
// caused mispositioned/panned audio.
class VASourceAmbient : public ALSourceNodeRelative
{
    GDCLASS(VASourceAmbient, ALSourceNodeRelative);

private:
    va_godot::VAWorld *va_world = nullptr;
    bool played = false;

    // Set while _enter_tree found no VAWorld yet - see VAEmitter's identical
    // waiting_for_world field for the rationale.
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceAmbient();
    ~VASourceAmbient();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    // Matches VASourceAmbient.cs's Play() override: refuses to start until
    // the listener has produced an ambient filter result at least once.
    bool play() override;
};
