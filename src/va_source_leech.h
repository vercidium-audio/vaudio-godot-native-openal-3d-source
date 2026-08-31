#pragma once

#include "al_source3d.h"

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

class VASourceLeech : public ALSource3D
{
    GDCLASS(VASourceLeech, ALSource3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Not owned - the parent VAEmitter this node leeches raytracing results off, resolved once in _enter_tree via get_parent().
    va_godot::VAEmitter *emitter = nullptr;

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

    // Matches VASource's Play() override: if the parent emitter hasn't produced raytracing results yet, returns false without playing.
    bool play() override;

    // Current muffling filter state (direct/dry path only); 1.0/1.0 until the listener has raytraced the parent emitter at least once.
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;
};
