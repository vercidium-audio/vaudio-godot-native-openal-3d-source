#include "al_source_node3d.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_source.h"

void ALSourceNode3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_stream"), &ALSourceNode3D::get_stream);
    ClassDB::bind_method(D_METHOD("set_stream", "value"), &ALSourceNode3D::set_stream);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");

    ClassDB::bind_method(D_METHOD("get_gain"), &ALSourceNode3D::get_gain);
    ClassDB::bind_method(D_METHOD("set_gain", "value"), &ALSourceNode3D::set_gain);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_gain", "get_gain");

    ClassDB::bind_method(D_METHOD("get_pitch"), &ALSourceNode3D::get_pitch);
    ClassDB::bind_method(D_METHOD("set_pitch", "value"), &ALSourceNode3D::set_pitch);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0.01,4.0,0.001"), "set_pitch", "get_pitch");

    ClassDB::bind_method(D_METHOD("get_looping"), &ALSourceNode3D::get_looping);
    ClassDB::bind_method(D_METHOD("set_looping", "value"), &ALSourceNode3D::set_looping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "looping"), "set_looping", "get_looping");

    ClassDB::bind_method(D_METHOD("get_relative"), &ALSourceNode3D::get_relative);
    ClassDB::bind_method(D_METHOD("set_relative", "value"), &ALSourceNode3D::set_relative);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "relative"), "set_relative", "get_relative");

    ClassDB::bind_method(D_METHOD("get_max_distance"), &ALSourceNode3D::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &ALSourceNode3D::set_max_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_max_distance", "get_max_distance");

    ClassDB::bind_method(D_METHOD("get_reference_distance"), &ALSourceNode3D::get_reference_distance);
    ClassDB::bind_method(D_METHOD("set_reference_distance", "value"), &ALSourceNode3D::set_reference_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_reference_distance", "get_reference_distance");
}

ALSourceNode3D::ALSourceNode3D()
{
}

ALSourceNode3D::~ALSourceNode3D()
{
}

void ALSourceNode3D::ensure_stream_decoded()
{
    if (stream == decoded_stream)
    {
        return;
    }

    if (stream.is_null())
    {
        decoded_stream = stream;
        return;
    }

    if (decoded_buffer.load(stream))
    {
        buffer_handle = decoded_buffer.get_handle();
        decoded_stream = stream;
    }
    else
    {
        UtilityFunctions::push_error("[vaudio-godot-native-openal] ", get_name(), ": failed to decode `stream`, nothing will play");
    }
}

bool ALSourceNode3D::play()
{
    ensure_stream_decoded();

    if (buffer_handle == 0)
    {
        return false;
    }

    auto source = std::make_unique<ALSource>();

    if (!source->create())
    {
        return false;
    }

    source->set_buffer(buffer_handle);
    source->set_gain(gain);
    source->set_pitch(pitch);
    source->set_looping(looping);
    source->set_relative(relative);
    source->set_max_distance(max_distance);
    source->set_reference_distance(reference_distance);
    source->set_position(get_global_position());

    ALuint reverb_slot = effect ? effect->get_slot_handle() : 0;
    source->set_direct_filter(filter.get_handle());
    source->set_reverb_send(reverb_slot); // fullReverb=true convention: unfiltered send

    source->play();

    sources.push_back(std::move(source));
    return true;
}

void ALSourceNode3D::_process(double delta)
{
    // Matches ALSource3D.cs's _Process: keeps every live (non-relative)
    // source's OpenAL position in sync with this node's current global
    // position every frame - play() only sets it once, at the moment the
    // source starts, so without this a source's audible position freezes at
    // wherever the node was when it started playing (or, for VASource, at
    // the origin if the node is moved before its emitter first finishes
    // raytracing and playback triggers).
    if (relative)
    {
        return;
    }

    Vector3 position = get_global_position();

    for (auto &source : sources)
    {
        source->set_position(position);
    }
}

void ALSourceNode3D::stop()
{
    for (auto &source : sources)
    {
        source->stop();
    }
}

bool ALSourceNode3D::is_playing() const
{
    for (auto &source : sources)
    {
        if (!source->is_finished())
        {
            return false;
        }
    }

    return true;
}

void ALSourceNode3D::update_filter(float new_gain, float new_gain_hf, bool fullReverb)
{
    if (!filter.is_valid())
    {
        filter.create(new_gain, new_gain_hf);
    }
    else
    {
        filter.set_gain(new_gain, new_gain_hf);
    }

    ALuint reverb_slot = effect ? effect->get_slot_handle() : 0;

    // fullReverb sends the clean/unfiltered signal to the reverb effect
    // (reverb_filter_handle left at ALSource::set_reverb_send's default 0) -
    // otherwise the same muffling filter also applies to the wet send.
    ALuint reverb_filter_handle = fullReverb ? 0 : filter.get_handle();

    for (auto &source : sources)
    {
        source->set_direct_filter(filter.get_handle());
        source->set_reverb_send(reverb_slot, reverb_filter_handle);
    }
}

void ALSourceNode3D::set_gain(float value)
{
    gain = value;

    for (auto &source : sources)
    {
        source->set_gain(value);
    }
}

void ALSourceNode3D::set_pitch(float value)
{
    pitch = value;

    for (auto &source : sources)
    {
        source->set_pitch(value);
    }
}

void ALSourceNode3D::set_looping(bool value)
{
    looping = value;

    for (auto &source : sources)
    {
        source->set_looping(value);
    }
}

void ALSourceNode3D::set_relative(bool value)
{
    relative = value;

    for (auto &source : sources)
    {
        source->set_relative(value);
    }
}

void ALSourceNode3D::set_max_distance(float value)
{
    max_distance = value;

    for (auto &source : sources)
    {
        source->set_max_distance(value);
    }
}

void ALSourceNode3D::set_reference_distance(float value)
{
    reference_distance = value;

    for (auto &source : sources)
    {
        source->set_reference_distance(value);
    }
}
