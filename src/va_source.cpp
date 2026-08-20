#include "va_source.h"

#include <godot_cpp/core/class_db.hpp>

void VASource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_play_when_raytracing_completes"), &VASource::get_play_when_raytracing_completes);
    ClassDB::bind_method(D_METHOD("set_play_when_raytracing_completes", "value"), &VASource::set_play_when_raytracing_completes);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "play_when_raytracing_completes"), "set_play_when_raytracing_completes", "get_play_when_raytracing_completes");
}

VASource::VASource()
{
}

VASource::~VASource()
{
}

bool VASource::get_play_when_raytracing_completes() const
{
    return play_when_raytracing_completes;
}

void VASource::set_play_when_raytracing_completes(bool value)
{
    play_when_raytracing_completes = value;
}

bool VASource::play()
{
    if (!is_raytraced())
    {
        play_when_raytracing_completes = true;
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

    if (!played && play_when_raytracing_completes)
    {
        play();
    }
}
