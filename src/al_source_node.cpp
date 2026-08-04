#include "al_source_node.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_source.h"

void ALSourceNode::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_stream"), &ALSourceNode::get_stream);
    ClassDB::bind_method(D_METHOD("set_stream", "value"), &ALSourceNode::set_stream);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");

    ClassDB::bind_method(D_METHOD("get_gain"), &ALSourceNode::get_gain);
    ClassDB::bind_method(D_METHOD("set_gain", "value"), &ALSourceNode::set_gain);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_gain", "get_gain");

    ClassDB::bind_method(D_METHOD("get_pitch"), &ALSourceNode::get_pitch);
    ClassDB::bind_method(D_METHOD("set_pitch", "value"), &ALSourceNode::set_pitch);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0.01,4.0,0.01"), "set_pitch", "get_pitch");

    ClassDB::bind_method(D_METHOD("get_looping"), &ALSourceNode::get_looping);
    ClassDB::bind_method(D_METHOD("set_looping", "value"), &ALSourceNode::set_looping);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "looping"), "set_looping", "get_looping");

    ClassDB::bind_method(D_METHOD("get_autoplay"), &ALSourceNode::get_autoplay);
    ClassDB::bind_method(D_METHOD("set_autoplay", "value"), &ALSourceNode::set_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("play"), &ALSourceNode::play);
    ClassDB::bind_method(D_METHOD("stop"), &ALSourceNode::stop);
    ClassDB::bind_method(D_METHOD("is_playing"), &ALSourceNode::is_playing);
}

ALSourceNode::ALSourceNode()
{
}

ALSourceNode::~ALSourceNode()
{
}

void ALSourceNode::_ready()
{
    if (autoplay && !Engine::get_singleton()->is_editor_hint())
    {
        play();
    }
}

void ALSourceNode::ensure_stream_decoded()
{
    if (stream == decoded_stream)
        return;

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
        UtilityFunctions::push_error("[vaudio-godot-native-openal] ", get_name(), ": failed to decode the stream");
    }
}

bool ALSourceNode::play()
{
    ensure_stream_decoded();

    // TODO - what's this about? Could buffer_handle be null? Why?
    if (buffer_handle == 0)
        return false;

    auto source = std::make_unique<ALSource>();

    // TODO - log warning that max number of sounds has been reached
    if (!source->create())
        return false;

    source->set_buffer(buffer_handle);
    source->set_gain(gain);
    source->set_pitch(pitch);
    source->set_looping(looping);

    configure_source(*source);

    ALuint reverb_slot = effect ? effect->get_slot_handle() : 0;
    source->set_direct_filter(filter.get_handle());
    source->set_reverb_send(reverb_slot);

    source->play();

    sources.push_back(std::move(source));
    return true;
}

void ALSourceNode::stop()
{
    for (auto &source : sources)
        source->stop();
}

bool ALSourceNode::is_playing() const
{
    for (auto &source : sources)
        if (!source->is_finished())
            return false;

    return true;
}

void ALSourceNode::update_filter(float new_gain, float new_gain_hf, bool fullReverb)
{
    if (!filter.is_valid())
        filter.create(new_gain, new_gain_hf);
    else
        filter.set_gain(new_gain, new_gain_hf);

    ALuint reverb_slot = effect ? effect->get_slot_handle() : 0;

    // if fullReverb=true, the clean/unfiltered sound is sent to the reverb effect, and then the reverb effect's output gain is reduced based on vaEAXReverbGetRelativeGain
    ALuint reverb_filter_handle = fullReverb ? 0 : filter.get_handle();

    for (auto &source : sources)
    {
        source->set_direct_filter(filter.get_handle());
        source->set_reverb_send(reverb_slot, reverb_filter_handle);
    }
}

void ALSourceNode::set_gain(float value)
{
    gain = value;

    for (auto &source : sources)
        source->set_gain(value);
}

void ALSourceNode::set_pitch(float value)
{
    pitch = value;

    for (auto &source : sources)
        source->set_pitch(value);
}

void ALSourceNode::set_looping(bool value)
{
    looping = value;

    for (auto &source : sources)
        source->set_looping(value);
}

void ALSourceNode::set_autoplay(bool value)
{
    autoplay = value;
}
