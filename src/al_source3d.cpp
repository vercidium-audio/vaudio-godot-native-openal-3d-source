#include "al_source3d.h"

#include <godot_cpp/core/class_db.hpp>

#include "openal/al_source_handle.h"

void ALSource3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_max_distance"), &ALSource3D::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &ALSource3D::set_max_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_max_distance", "get_max_distance");

    ClassDB::bind_method(D_METHOD("get_reference_distance"), &ALSource3D::get_reference_distance);
    ClassDB::bind_method(D_METHOD("set_reference_distance", "value"), &ALSource3D::set_reference_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_reference_distance", "get_reference_distance");

    // Script-only alias for `reference_distance` - not exposed in the inspector, see al_source3d.h's get_unit_size().
    ClassDB::bind_method(D_METHOD("get_unit_size"), &ALSource3D::get_unit_size);
    ClassDB::bind_method(D_METHOD("set_unit_size", "value"), &ALSource3D::set_unit_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_size", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_unit_size", "get_unit_size");
}

ALSource3D::ALSource3D()
{
}

ALSource3D::~ALSource3D()
{
}

void ALSource3D::configure_source(ALSourceHandle &source)
{
    source.set_max_distance(max_distance);
    source.set_reference_distance(reference_distance);
    source.set_position(get_global_position());
}

void ALSource3D::_process(double delta)
{
    ALSource::_process(delta);

    // Keeps every live source's OpenAL position in sync each frame; play() only sets it once, so without this a source's audible position freezes.
    Vector3 position = get_global_position();

    for (auto &source : get_sources())
    {
        source->set_position(position);
    }
}

void ALSource3D::set_max_distance(float value)
{
    max_distance = value;

    for (auto &source : get_sources())
    {
        source->set_max_distance(value);
    }
}

void ALSource3D::set_reference_distance(float value)
{
    reference_distance = value;

    for (auto &source : get_sources())
    {
        source->set_reference_distance(value);
    }
}
