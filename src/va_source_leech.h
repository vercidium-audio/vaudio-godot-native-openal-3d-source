#pragma once

#include "al_source_node3d.h"

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{
class VAWorld;
class VAEmitter;
}

// A VASource that leeches raytracing results off a parent VAEmitter instead
// of owning/casting its own rays - an enemy might have a
// VAEmitter constantly raytraced, and multiple VASourceLeech children (
// footsteps, gunshots, etc.) can be heard through it without each casting
// their own rays or being individually registered as a target of the
// listener (see the unreal plugin's equivalent leech pattern).
class VASourceLeech : public ALSourceNode3D
{
    GDCLASS(VASourceLeech, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Not owned - the parent VAEmitter this node leeches raytracing results
    // off. Resolved once in _enter_tree via get_parent(), unlike VASource's
    // owned child emitter, this is never created/destroyed by this node.
    va_godot::VAEmitter *emitter = nullptr;

    bool play_when_raytracing_completes = true;
    bool played = false;

    void apply_raytracing_results(va_godot::VAEmitter *other);

protected:
    static void _bind_methods();

public:
    VASourceLeech();
    ~VASourceLeech();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    bool is_raytraced() const;

    // Matches VASource's Play() override: if the parent emitter hasn't
    // produced raytracing results yet, just arms
    // play_when_raytracing_completes and returns false - actual playback
    // happens once is_raytraced() is true.
    bool play() override;

    bool get_play_when_raytracing_completes() const;
    void set_play_when_raytracing_completes(bool value);

    // Current muffling filter state (direct/dry path only), read for
    // stats/debug display. 1.0/1.0 until the listener has raytraced the
    // parent emitter at least once.
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;
};
