#pragma once

#include "al_functions.h"

#include <deque>
#include <mutex>
#include <vector>

// Owns one OpenAL buffer whose samples are pulled from a callback instead of
// a fixed alBufferData upload - the native C++ equivalent of
// openal_soft_bindings's managed/ALStreamSource.cs's ALStreamSourceBuffer +
// OnBufferCallback pattern (godot_stream_plan.md section 5.2). Used by
// VAStreamSource (src/va_stream_source.h) to let a script push live PCM
// chunks (e.g. microphone/VOIP audio) into a source while it plays, instead
// of decoding one fixed-length AudioStream up front like ALBuffer does.
//
// OpenAL Soft can invoke the buffer callback from its own mixer thread, not
// just the main thread the rest of this plugin's AL calls run on - so
// pending_chunks/used_chunks are guarded by a mutex, matching why
// ALStreamSource.cs uses BlockingCollection instead of a plain queue.
class ALStreamBuffer
{
private:
    // One caller-pushed chunk of raw PCM bytes, plus how much of it is still
    // unconsumed by the callback - mirrors ALStreamSourceBuffer.cs's
    // data/offset/length fields (no separate "original" fields here since
    // this chunk is discarded, not recycled, once fully consumed - the
    // caller is expected to free/reuse its own copy after try_get_used_chunk
    // hands it back).
    struct Chunk
    {
        std::vector<uint8_t> data;
        int offset = 0;
        int length = 0;
    };

    ALuint handle = 0;

    std::mutex chunks_mutex;
    std::deque<Chunk> pending_chunks;
    std::deque<Chunk> used_chunks;

    // Chunk the callback only partially consumed on a previous call, carried
    // over so the next callback call resumes where it left off - mirrors
    // ALStreamSource.cs's partialBuffer field. Only ever touched from the AL
    // mixer thread inside on_buffer_callback, so it needs no locking of its
    // own.
    Chunk partial_chunk;
    bool has_partial_chunk = false;

    // Free function OpenAL invokes (ALBUFFERCALLBACKTYPESOFT signature) -
    // userptr is always `this` (passed to alBufferCallbackSOFT in create()),
    // so it forwards straight into on_buffer_callback.
    static ALsizei AL_APIENTRY buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) noexcept;

    // Instance-side implementation of the callback - copies from
    // pending_chunks (and any carried-over partial_chunk) into sampledata
    // until numbytes is satisfied or pending_chunks runs dry, in which case
    // the remainder is filled with silence. Fully-consumed chunks are moved
    // to used_chunks for the caller to reclaim via try_get_used_chunk().
    // Matches ALStreamSource.cs's OnBufferCallback step-by-step.
    ALsizei on_buffer_callback(ALvoid *sampledata, ALsizei numbytes);

public:
    ALStreamBuffer() = default;
    ~ALStreamBuffer();

    ALStreamBuffer(const ALStreamBuffer &) = delete;
    ALStreamBuffer &operator=(const ALStreamBuffer &) = delete;

    // Allocates the OpenAL buffer and registers this instance's callback via
    // alBufferCallbackSOFT. Returns false (with a Godot error already
    // logged) if AL_SOFT_callback_buffer isn't available on this device
    // (ALManager::al_buffer_callback_soft() is null) or buffer creation
    // fails - handle() stays 0 in that case. If this instance already owned a
    // buffer, it's destroyed first (see destroy()).
    bool create(ALenum format, int frequency);

    // Deletes the OpenAL buffer (if any) and drops every pending/used/partial
    // chunk. Safe to call more than once and safe to call even if create()
    // was never called or failed. Also called by the destructor.
    void destroy();

    // Copies length bytes starting at data into a new pending chunk. Safe to
    // call from any thread. The caller retains ownership of data - it's
    // copied immediately, so data can be freed/reused right after this call
    // returns.
    void enqueue(const uint8_t *data, int length);

    // Pops one fully-consumed chunk into out_data/out_length, if one is
    // ready, and returns true - otherwise returns false and leaves the
    // outputs untouched. Call in a loop until it returns false to drain
    // every ready chunk (matches ALStreamSource.cs's TryGetUsedData drain
    // pattern in VAStreamSource::_process). The returned bytes are a copy
    // owned by the caller (out_data is moved out of this ALStreamBuffer's
    // internal storage) - no separate free call is needed.
    bool try_get_used_chunk(std::vector<uint8_t> &out_data);

    ALuint get_handle() const
    {
        return handle;
    }

    bool is_valid() const
    {
        return handle != 0;
    }
};
