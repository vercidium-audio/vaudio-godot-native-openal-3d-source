#include "al_source_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "openal/al_source.h"
#include "va_engine_util.h"

void ALSourceNode::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_streams"), &ALSourceNode::get_streams);
    ClassDB::bind_method(D_METHOD("set_streams", "value"), &ALSourceNode::set_streams);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "streams", PROPERTY_HINT_TYPE_STRING, String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":AudioStream"), "set_streams", "get_streams");

    ClassDB::bind_method(D_METHOD("get_gain"), &ALSourceNode::get_gain);
    ClassDB::bind_method(D_METHOD("set_gain", "value"), &ALSourceNode::set_gain);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_gain", "get_gain");

    ClassDB::bind_method(D_METHOD("get_pitch"), &ALSourceNode::get_pitch);
    ClassDB::bind_method(D_METHOD("set_pitch", "value"), &ALSourceNode::set_pitch);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0.01,4.0,0.01"), "set_pitch", "get_pitch");

    ClassDB::bind_method(D_METHOD("get_pitch_randomness"), &ALSourceNode::get_pitch_randomness);
    ClassDB::bind_method(D_METHOD("set_pitch_randomness", "value"), &ALSourceNode::set_pitch_randomness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_randomness", PROPERTY_HINT_RANGE, "1.0,4.0,0.01"), "set_pitch_randomness", "get_pitch_randomness");

    ClassDB::bind_method(D_METHOD("get_volume_randomness_db"), &ALSourceNode::get_volume_randomness_db);
    ClassDB::bind_method(D_METHOD("set_volume_randomness_db", "value"), &ALSourceNode::set_volume_randomness_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_randomness_db", PROPERTY_HINT_RANGE, "0.0,24.0,0.1"), "set_volume_randomness_db", "get_volume_randomness_db");

    ClassDB::bind_method(D_METHOD("get_playback_no_repeat"), &ALSourceNode::get_playback_no_repeat);
    ClassDB::bind_method(D_METHOD("set_playback_no_repeat", "value"), &ALSourceNode::set_playback_no_repeat);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playback_no_repeat"), "set_playback_no_repeat", "get_playback_no_repeat");

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
    // A decode task holds a raw pointer to decoding_buffer via userdata (see
    // ensure_stream_decode_started()) - must not let it run after this node
    // (and decoding_buffer with it) is destroyed.
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
    node->decode_succeeded = node->decoding_buffer.decode(node->decoding_stream);
}

bool ALSourceNode::ensure_stream_decode_started()
{
    if (streams.is_empty())
    {
        decoded_buffers.clear();
        decoded_streams.clear();
        pending_streams.clear();
        return false;
    }

    // Drop any cached decode whose stream is no longer in `streams` (e.g. an
    // entry was removed or reassigned).
    for (size_t i = 0; i < decoded_streams.size();)
    {
        bool still_wanted = false;

        for (int j = 0; j < streams.size(); j++)
        {
            if (Ref<AudioStream>(streams[j]) == decoded_streams[i])
            {
                still_wanted = true;
                break;
            }
        }

        if (still_wanted)
        {
            i++;
        }
        else
        {
            decoded_streams.erase(decoded_streams.begin() + i);
            decoded_buffers.erase(decoded_buffers.begin() + i);
        }
    }

    // Queue a decode for every stream that isn't already decoded, in flight,
    // or already queued.
    for (int i = 0; i < streams.size(); i++)
    {
        Ref<AudioStream> wanted = streams[i];

        if (wanted.is_null())
            continue;

        bool already_decoded = false;

        for (const Ref<AudioStream> &decoded : decoded_streams)
        {
            if (decoded == wanted)
            {
                already_decoded = true;
                break;
            }
        }

        if (already_decoded || wanted == decoding_stream)
            continue;

        bool already_queued = false;

        for (const Ref<AudioStream> &pending : pending_streams)
        {
            if (pending == wanted)
            {
                already_queued = true;
                break;
            }
        }

        if (!already_queued)
            pending_streams.push_back(wanted);
    }

    if (pending_streams.empty() && decode_task_id == WorkerThreadPool::INVALID_TASK_ID)
        return true;

    // A decode is already in flight - let it (and the rest of the queue)
    // finish rather than starting a second, redundant one.
    if (decode_task_id != WorkerThreadPool::INVALID_TASK_ID)
        return true;

    decoding_stream = pending_streams.front();
    pending_streams.erase(pending_streams.begin());
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

    if (decode_succeeded && decoding_buffer.upload())
    {
        decoded_streams.push_back(decoding_stream);
        decoded_buffers.push_back(std::move(decoding_buffer));
        decoding_buffer = ALBuffer();
    }
    else
    {
        String stream_path = decoding_stream.is_valid() ? decoding_stream->get_path() : String();

        if (stream_path.is_empty())
            stream_path = "<no resource path, likely a stream created at runtime>";

        if (decode_succeeded)
            VA_ERROR_NAMED("Failed to upload the decoded stream to OpenAL: ", stream_path);
        else
            VA_ERROR_NAMED("Failed to decode the stream: ", stream_path);
    }

    decoding_stream.unref();

    // More streams still queued (e.g. multiple new entries in `streams`) -
    // start the next one rather than waiting for another play()/poll cycle.
    if (!pending_streams.empty())
    {
        decoding_stream = pending_streams.front();
        pending_streams.erase(pending_streams.begin());
        decode_task_id = WorkerThreadPool::get_singleton()->add_native_task(
            &ALSourceNode::decode_stream_task, this, false, "vaudio stream decode");
        return;
    }

    if (play_requested)
    {
        play_requested = false;
        start_playing();
    }
}

int ALSourceNode::pick_stream_index() const
{
    if (decoded_buffers.empty())
        return -1;

    if (decoded_buffers.size() == 1)
        return 0;

    int index = (int)UtilityFunctions::randi_range(0, (int64_t)decoded_buffers.size() - 1);

    if (playback_no_repeat && index == last_played_index)
        index = (index + 1) % (int)decoded_buffers.size();

    return index;
}

bool ALSourceNode::start_playing()
{
    int index = pick_stream_index();

    // No decoded buffer to play (streams failed to decode/upload, see
    // poll_decode_task(), or nothing was ever assigned).
    if (index < 0)
        return false;

    last_played_index = index;
    buffer_handle = decoded_buffers[index].get_handle();

    if (buffer_handle == 0)
        return false;

    auto source = std::make_unique<ALSource>();

    // ALSource::create() already logs the reason for failure (e.g. the
    // OpenAL source limit has been reached).
    if (!source->create())
        return false;

    // Matches AudioStreamRandomizer's random_pitch/random_volume_offset_db:
    // pitch_randomness of 1.0 (no variation) and volume_randomness_db of 0.0
    // (no variation) both collapse randf_range to their single input value.
    float randomized_pitch = pitch * (float)UtilityFunctions::randf_range(1.0 / pitch_randomness, pitch_randomness);
    float randomized_gain = gain * (float)UtilityFunctions::db_to_linear(UtilityFunctions::randf_range(-volume_randomness_db, volume_randomness_db));

    source->set_buffer(buffer_handle);
    source->set_gain(randomized_gain);
    source->set_pitch(randomized_pitch);
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
    bool need_decode = !ensure_stream_decode_started();

    if (need_decode)
        return false;

    if (decode_task_id == WorkerThreadPool::INVALID_TASK_ID && pending_streams.empty())
        return start_playing();

    // A decode is in flight - poll_decode_task() will start playback once
    // every entry in `streams` has finished decoding.
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
