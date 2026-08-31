#include "va_source.h"

#include <godot_cpp/core/class_db.hpp>

void VASource::_bind_methods()
{
}

VASource::VASource()
{
}

VASource::~VASource()
{
}

bool VASource::play()
{
    if (!is_raytraced())
    {
        return false;
    }

    played = VARaytracedSource::play();

    return played;
}

void VASource::_process(double delta)
{
    VARaytracedSource::_process(delta);
    process_raytracing(delta);

    if (!is_raytraced())
    {
        return;
    }

    if (!played && get_autoplay())
    {
        play();
    }
}
