#pragma once

#include "al_source.h"

using namespace godot;

class ALSourceHandle;

// Spatialised AL source
class ALSource3D : public ALSource
{
    GDCLASS(ALSource3D, ALSource);

private:
    float max_distance = 100.0f;
    float reference_distance = 8.0f;

protected:
    static void _bind_methods();

    void configure_source(ALSourceHandle &source) override;

public:
    ALSource3D();
    ~ALSource3D();

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

    // Script-only alias for `reference_distance`, matching AudioStreamPlayer3D's
    // `unit_size` - lets a script written against AudioStreamPlayer3D keep
    // working unmodified after converting to this node (see
    // va_conversion_plugin.cpp, which does the same remap at conversion time).
    float get_unit_size() const
    {
        return get_reference_distance();
    }

    void set_unit_size(float value)
    {
        set_reference_distance(value);
    }
};
