#include "al_stream_buffer.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <algorithm>
#include <cstring>

ALStreamBuffer::~ALStreamBuffer()
{
    destroy();
}

void ALStreamBuffer::destroy()
{
    ALManager *manager = ALManager::get_singleton();

    if (handle != 0 && manager)
    {
        ALuint handles[1] = {handle};
        manager->al_delete_buffers()(1, handles);
    }

    handle = 0;

    std::lock_guard<std::mutex> lock(chunks_mutex);
    pending_chunks.clear();
    used_chunks.clear();
    has_partial_chunk = false;
}

bool ALStreamBuffer::create(ALenum format, int frequency)
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->is_initialized())
    {
        VA_ERROR("ALStreamBuffer::create called before the OpenAL device is initialized");
        return false;
    }

    if (!manager->al_buffer_callback_soft())
    {
        VA_ERROR("AL_SOFT_callback_buffer isn't available on this device - streaming sources aren't supported");
        return false;
    }

    destroy();

    ALuint new_handle = 0;
    manager->al_gen_buffers()(1, &new_handle);

    if (new_handle == 0)
    {
        VA_ERROR("alGenBuffers failed, AL error ", (int)manager->al_get_error()());
        return false;
    }

    manager->al_buffer_callback_soft()(new_handle, format, frequency, &ALStreamBuffer::buffer_callback, this);

    ALenum error = manager->al_get_error()();

    if (error != AL_NO_ERROR)
    {
        VA_ERROR("alBufferCallbackSOFT failed: ", ALErrorToString(error));
        ALuint handles[1] = {new_handle};
        manager->al_delete_buffers()(1, handles);
        return false;
    }

    handle = new_handle;
    return true;
}

void ALStreamBuffer::enqueue(const uint8_t *data, int length)
{
    if (length <= 0)
    {
        return;
    }

    Chunk chunk;
    chunk.data.assign(data, data + length);
    chunk.offset = 0;
    chunk.length = length;

    std::lock_guard<std::mutex> lock(chunks_mutex);
    pending_chunks.push_back(std::move(chunk));
}

bool ALStreamBuffer::try_get_used_chunk(std::vector<uint8_t> &out_data)
{
    std::lock_guard<std::mutex> lock(chunks_mutex);

    if (used_chunks.empty())
    {
        return false;
    }

    out_data = std::move(used_chunks.front().data);
    used_chunks.pop_front();
    return true;
}

ALsizei AL_APIENTRY ALStreamBuffer::buffer_callback(ALvoid *userptr, ALvoid *sampledata, ALsizei numbytes) noexcept
{
    return static_cast<ALStreamBuffer *>(userptr)->on_buffer_callback(sampledata, numbytes);
}

ALsizei ALStreamBuffer::on_buffer_callback(ALvoid *sampledata, ALsizei numbytes)
{
    uint8_t *write = static_cast<uint8_t *>(sampledata);
    ALsizei bytes_left = numbytes;

    while (bytes_left > 0)
    {
        Chunk chunk;

        if (has_partial_chunk)
        {
            chunk = std::move(partial_chunk);
            has_partial_chunk = false;
        }
        else
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);

            if (pending_chunks.empty())
            {
                break;
            }

            chunk = std::move(pending_chunks.front());
            pending_chunks.pop_front();
        }

        int bytes_to_write = std::min(chunk.length, (int)bytes_left);

        std::memcpy(write, chunk.data.data() + chunk.offset, (size_t)bytes_to_write);

        write += bytes_to_write;
        bytes_left -= bytes_to_write;

        if (bytes_to_write < chunk.length)
        {
            // Only partially consumed - keep the remainder for the next
            // callback call instead of moving it to used_chunks yet.
            chunk.offset += bytes_to_write;
            chunk.length -= bytes_to_write;
            partial_chunk = std::move(chunk);
            has_partial_chunk = true;
        }
        else
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);
            used_chunks.push_back(std::move(chunk));
        }
    }

    // Not enough pending data to fill the request - pad the rest with
    // silence rather than starving the source.
    if (bytes_left > 0)
    {
        std::memset(write, 0, (size_t)bytes_left);
    }

    return numbytes;
}
