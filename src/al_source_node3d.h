#pragma once

#include "al_source_node.h"

using namespace godot;

class ALSource;

// Positional/spatialized ALSourceNode - the native C++ equivalent of
// godot-openal's nodes/ALSource3D.cs, the base VASource extends
// (native_godot_plan.md "Implement VASource's per-frame result application").
// Sources created here are heard at this node's actual world position, with
// OpenAL's inverse-distance attenuation applied (see max_distance/
// reference_distance below). For sounds that should always be heard as if
// right at the listener regardless of node position (UI/footstep/ambience-
// style), see ALSourceNodeRelative instead - the two used to be a single
// class toggled by a `relative` bool, but that let a stray world-space
// position leak into AL_SOURCE_RELATIVE's listener-local interpretation and
// pan the "relative" sound off to one side, so they're now fully separate
// classes.
class ALSourceNode3D : public ALSourceNode
{
    GDCLASS(ALSourceNode3D, ALSourceNode);

private:
    // AL_INVERSE_DISTANCE_CLAMPED's clamp bounds (see ALManager::initialize).
    // max_distance=0 is a degenerate clamp (distance is clamped to
    // [reference_distance, 0], i.e. never allowed past reference_distance),
    // so this needs a real default rather than OpenAL's own alSourcef
    // default of 0 - 100 world units is a reasonable "basically inaudible
    // past here" default for a 3D game scene.
    float max_distance = 100.0f;
    float reference_distance = 1.0f;

protected:
    static void _bind_methods();

    // ALSourceNode::play() hook: stamps the freshly-created source with this
    // node's current global position and the distance-model knobs below.
    void configure_source(ALSource &source) override;

public:
    ALSourceNode3D();
    ~ALSourceNode3D();

    void _process(double delta) override;

    void set_max_distance(float value);
    void set_reference_distance(float value);

    float get_max_distance() const
    {
        return max_distance;
    }

    float get_reference_distance() const
    {
        return reference_distance;
    }
};
