#include "al_source_node3d.h"

#include <godot_cpp/core/class_db.hpp>

#include "openal/al_source.h"

void ALSourceNode3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_max_distance"), &ALSourceNode3D::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &ALSourceNode3D::set_max_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_max_distance", "get_max_distance");

    ClassDB::bind_method(D_METHOD("get_reference_distance"), &ALSourceNode3D::get_reference_distance);
    ClassDB::bind_method(D_METHOD("set_reference_distance", "value"), &ALSourceNode3D::set_reference_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_reference_distance", "get_reference_distance");

    // Script-only alias for `reference_distance` - not exposed in the inspector,
    // see get_unit_size()'s comment in al_source_node3d.h for why this exists.
    ClassDB::bind_method(D_METHOD("get_unit_size"), &ALSourceNode3D::get_unit_size);
    ClassDB::bind_method(D_METHOD("set_unit_size", "value"), &ALSourceNode3D::set_unit_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unit_size", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_unit_size", "get_unit_size");
}

ALSourceNode3D::ALSourceNode3D()
{
}

ALSourceNode3D::~ALSourceNode3D()
{
}

void ALSourceNode3D::configure_source(ALSource &source)
{
    source.set_max_distance(max_distance);
    source.set_reference_distance(reference_distance);
    source.set_position(get_global_position());
}

void ALSourceNode3D::_process(double delta)
{
    ALSourceNode::_process(delta);

    // Matches ALSource3D.cs's _Process: keeps every live source's OpenAL
    // position in sync with this node's current global position every frame
    // - play() only sets it once, at the moment the source starts, so
    // without this a source's audible position freezes at wherever the node
    // was when it started playing (or, for VASource, at the origin if the
    // node is moved before its emitter first finishes raytracing and
    // playback triggers).
    Vector3 position = get_global_position();

    for (auto &source : get_sources())
    {
        source->set_position(position);
    }
}

void ALSourceNode3D::set_max_distance(float value)
{
    max_distance = value;

    for (auto &source : get_sources())
    {
        source->set_max_distance(value);
    }
}

void ALSourceNode3D::set_reference_distance(float value)
{
    reference_distance = value;

    for (auto &source : get_sources())
    {
        source->set_reference_distance(value);
    }
}
