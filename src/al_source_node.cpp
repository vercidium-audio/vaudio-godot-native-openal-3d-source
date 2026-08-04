#include "al_source_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_source.h"
#include "va_engine_util.h"

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
    // A decode task holds a raw pointer to decoded_buffer via userdata (see
    // ensure_stream_decode_started()) - must not let it run after this node
    // (and decoded_buffer with it) is destroyed.
    if (decode_task_id != WorkerThreadPool::INVALID_TASK_ID)
        WorkerThreadPool::get_singleton()->wait_for_task_completion(decode_task_id);
}

void ALSourceNode::_ready()
{
    if (autoplay && !IS_EDITOR_HINT())
    {
        play();
    }
}

void ALSourceNode::_process(double delta)
{
    poll_decode_task();
}

void ALSourceNode::decode_stream_task(void *userdata)
{
    ALSourceNode *node = static_cast<ALSourceNode *>(userdata);
    node->decode_succeeded = node->decoded_buffer.decode(node->decoding_stream);
}

bool ALSourceNode::ensure_stream_decode_started()
{
    // AudioStreamRandomizer picks a sub-stream inside instantiate_playback(),
    // so it must be re-decoded on every play() to get a fresh random pick -
    // caching it like a regular stream would freeze on whichever sub-stream
    // was picked first.
    bool is_randomizer = Object::cast_to<AudioStreamRandomizer>(stream.ptr()) != nullptr;

    if (!is_randomizer && stream == decoded_stream)
        return true;

    if (stream.is_null())
    {
        decoded_stream = stream;
        return false;
    }

    // A decode for this exact stream is already in flight - let it finish
    // rather than starting a second, redundant one.
    if (decode_task_id != WorkerThreadPool::INVALID_TASK_ID)
        return true;

    decoding_stream = stream;
    decode_task_id = WorkerThreadPool::get_singleton()->add_native_task(
        &ALSourceNode::decode_stream_task, this, false, "vaudio stream decode");

    return true;
}

void ALSourceNode::poll_decode_task()
{
    if (decode_task_id == WorkerThreadPool::INVALID_TASK_ID)
        return;

    if (!WorkerThreadPool::get_singleton()->is_task_completed(decode_task_id))
        return;

    WorkerThreadPool::get_singleton()->wait_for_task_completion(decode_task_id);
    decode_task_id = WorkerThreadPool::INVALID_TASK_ID;

    if (decode_succeeded && decoded_buffer.upload())
    {
        buffer_handle = decoded_buffer.get_handle();
        decoded_stream = decoding_stream;
    }
    else
    {
        VA_ERROR_NAMED("failed to decode the stream");
    }

    decoding_stream.unref();

    if (play_requested)
    {
        play_requested = false;
        start_playing();
    }
}

bool ALSourceNode::start_playing()
{
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

bool ALSourceNode::play()
{
    bool is_randomizer = Object::cast_to<AudioStreamRandomizer>(stream.ptr()) != nullptr;
    bool already_decoded = !is_randomizer && stream == decoded_stream && stream.is_valid();

    if (!ensure_stream_decode_started())
        return false;

    if (already_decoded)
        return start_playing();

    // Stream isn't decoded yet (or is a randomizer re-rolling) - a decode
    // task is now in flight (or already was); poll_decode_task() will start
    // playback once it finishes.
    play_requested = true;
    return true;
}

void ALSourceNode::stop()
{
    play_requested = false;

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
