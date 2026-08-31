#pragma once

#include "va_raytraced_source.h"

using namespace godot;

class VASource : public VARaytracedSource
{
    GDCLASS(VASource, VARaytracedSource);

private:
    bool played = false;

protected:
    static void _bind_methods();

public:
    VASource();
    ~VASource();

    void _process(double delta) override;

    bool play() override;
};
