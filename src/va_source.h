#pragma once

#include "va_raytraced_source.h"

using namespace godot;

// A spatialised 3D sound that casts its own rays and plays a fixed,
// decoded-once AudioStream (see ALSource's streams/play() machinery).
// All raytracing/reverb/muffling/ambience behaviour lives on
// VARaytracedSource - this class only adds the "defer play() until
// raytracing has produced results" behaviour specific to fixed-buffer
// playback.
class VASource : public VARaytracedSource
{
    GDCLASS(VASource, VARaytracedSource);

private:
    bool play_when_raytracing_completes = true;
    bool played = false;

protected:
    static void _bind_methods();

public:
    VASource();
    ~VASource();

    void _process(double delta) override;

    // If raytracing hasn't produced results yet, arms play_when_raytracing_completes and returns false. Actual playback happens once is_raytraced() is true.
    bool play() override;

    bool get_play_when_raytracing_completes() const;
    void set_play_when_raytracing_completes(bool value);
};
