#pragma once

#include <godot_cpp/variant/string.hpp>

#include "al_functions.h"

#include <functional>
#include <vector>

using namespace godot;

// Opens an OpenAL input (microphone) capture device and polls it for new samples, native equivalent of ALCaptureDevice.cs.
// Deliberately not a Godot Node: a plain object owned by whichever script/node wants microphone input.
//
// Typical usage:
//   ALCaptureDevice mic;
//   mic.open("", AL_FORMAT_MONO16, 44100, 1024, [&](const uint8_t *data, int num_bytes) {
//       stream_source->push_audio_data(data, num_bytes);
//   });
//   mic.start();
//   // each frame:
//   mic.update();
class ALCaptureDevice
{
private:
    ALCdevice *device = nullptr;
    ALenum format = AL_FORMAT_MONO16;
    int buffer_size_frames = 0;
    int bytes_per_frame = 0;

    std::vector<uint8_t> sample_buffer;

    // Called from update() with (data, num_bytes) once per poll that finds new samples; reports a byte count (not frame count) so callers don't
    // need to know this device's bytes-per-frame. data points into sample_buffer, valid only for the duration of the callback.
    std::function<void(const uint8_t *, int)> data_callback;

    bool capturing = false;

public:
    ALCaptureDevice() = default;
    ~ALCaptureDevice();

    ALCaptureDevice(const ALCaptureDevice &) = delete;
    ALCaptureDevice &operator=(const ALCaptureDevice &) = delete;

    // Opens device_name (empty for the driver's default - see ALManager::get_available_capture_devices()) at the given format/frequency, with an
    // internal ring buffer sized for buffer_size_frames frames. callback is invoked from update() with new samples. Only mono/stereo 8/16-bit PCM is supported.
    bool open(const String &device_name, ALenum format, int frequency, int buffer_size_frames,
        std::function<void(const uint8_t *, int)> callback);

    // Starts the device capturing into its internal ring buffer; update() must still be called each frame to pull samples out and reach data_callback.
    void start();

    // Stops capturing. Already-buffered samples remain available to update()/alcCaptureSamples until read.
    void stop();

    // Reads however many frames are currently available (capped at buffer_size_frames) and, if any, invokes data_callback. Call once per frame.
    void update();

    // Stops capturing (if active) and closes the device. Safe to call more than once, and even if open() was never called or failed.
    void close();

    bool is_valid() const
    {
        return device != nullptr;
    }
};
