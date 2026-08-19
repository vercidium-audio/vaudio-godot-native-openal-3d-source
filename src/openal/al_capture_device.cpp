#include "al_capture_device.h"

#include "../va_engine_util.h"
#include "al_manager.h"

#include <algorithm>

// Only the formats a microphone realistically captures in - unlike
// ALHelpers.cs's GetBytesPerFrame (which also covers quad/5.1/7.1/B-format
// output layouts that don't apply to capture), this stays scoped to what
// open() actually accepts. Returns 0 for any other format.
static int bytes_per_frame_for_format(ALenum format)
{
    switch (format)
    {
        case AL_FORMAT_MONO8: return 1;
        case AL_FORMAT_MONO16: return 2;
        case AL_FORMAT_STEREO8: return 2;
        case AL_FORMAT_STEREO16: return 4;
        default: return 0;
    }
}

ALCaptureDevice::~ALCaptureDevice()
{
    close();
}

bool ALCaptureDevice::open(const String &device_name, ALenum p_format, int frequency, int p_buffer_size_frames,
    std::function<void(const uint8_t *, int)> callback)
{
    close();

    ALManager *manager = ALManager::get_singleton();

    if (!manager || !manager->alc_capture_open_device())
    {
        VA_ERROR("ALCaptureDevice::open called before ALManager has resolved its ALC_EXT_capture entry points");
        return false;
    }

    int frame_bytes = bytes_per_frame_for_format(p_format);

    if (frame_bytes == 0)
    {
        VA_ERROR("ALCaptureDevice::open called with an unsupported format - only mono/stereo 8/16-bit PCM are supported");
        return false;
    }

    if (!callback)
    {
        VA_ERROR("ALCaptureDevice::open called with no data callback");
        return false;
    }

    CharString device_name_utf8 = device_name.utf8();
    const char *device_name_cstr = device_name_utf8.length() > 0 ? device_name_utf8.get_data() : nullptr;

    ALCdevice *new_device = manager->alc_capture_open_device()(
        device_name_cstr, (ALCuint)frequency, p_format, (ALCsizei)p_buffer_size_frames);

    if (!new_device)
    {
        VA_ERROR("alcCaptureOpenDevice failed to open the input device: ",
            device_name.is_empty() ? String("<default>") : device_name);
        return false;
    }

    device = new_device;
    format = p_format;
    bytes_per_frame = frame_bytes;
    buffer_size_frames = p_buffer_size_frames;
    data_callback = std::move(callback);
    sample_buffer.resize((size_t)buffer_size_frames * bytes_per_frame);

    return true;
}

void ALCaptureDevice::start()
{
    if (!device)
    {
        VA_ERROR("ALCaptureDevice::start called on a device that isn't open");
        return;
    }

    ALManager::get_singleton()->alc_capture_start()(device);
    capturing = true;
}

void ALCaptureDevice::stop()
{
    if (!device)
    {
        return;
    }

    ALManager::get_singleton()->alc_capture_stop()(device);
    capturing = false;
}

void ALCaptureDevice::update()
{
    if (!device)
    {
        return;
    }

    ALManager *manager = ALManager::get_singleton();

    ALCint available_frames = 0;
    manager->alc_get_integerv()(device, ALC_CAPTURE_SAMPLES, 1, &available_frames);

    if (available_frames <= 0)
    {
        return;
    }

    // If more frames are available than sample_buffer can hold, only read
    // buffer_size_frames worth this call - the rest stays queued in the
    // driver's own capture ring buffer for the next update().
    int frames_to_read = std::min((int)available_frames, buffer_size_frames);

    manager->alc_capture_samples()(device, sample_buffer.data(), (ALCsizei)frames_to_read);

    data_callback(sample_buffer.data(), frames_to_read * bytes_per_frame);
}

void ALCaptureDevice::close()
{
    if (!device)
    {
        return;
    }

    if (capturing)
    {
        stop();
    }

    ALManager::get_singleton()->alc_capture_close_device()(device);
    device = nullptr;

    sample_buffer.clear();
    sample_buffer.shrink_to_fit();
    data_callback = nullptr;
}
