#pragma once

#include "al_source_relative.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

// Port of VASourceAmbient.cs. Reads the listener's already-computed ambient (non-directional) muffling gain rather than owning
// its own VAEmitter - for background ambience that should be muffled by the listener's current room without being raytraced
// itself. Waits for the listener's first ambient filter result before playing, then re-applies the gain every frame. Extends
// ALSourceRelative (not ALSource3D) so its sources stay AL_SOURCE_RELATIVE with a pinned origin (see that class's comment).
class VASourceAmbient : public ALSourceRelative
{
    GDCLASS(VASourceAmbient, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;
    bool played = false;

    // Set while _enter_tree found no VAWorld yet (see VAEmitter's identical waiting_for_world field).
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

    // Matches VASourceAmbient.cs's Play() override: refuses to start until the listener has produced an ambient filter result at least once.
    bool play() override;
};
