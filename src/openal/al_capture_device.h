#pragma once

#include <godot_cpp/variant/string.hpp>

#include "al_functions.h"

#include <functional>
#include <vector>

using namespace godot;

// Opens an OpenAL input (microphone) capture device and polls it for new
// samples - the native C++ equivalent of openal_soft_bindings's
// managed/ALCaptureDevice.cs (godot_stream_plan.md section 6). Deliberately
// not a Godot Node: it's a plain object owned by whichever script/node wants
// microphone input, matching ALCaptureDevice.cs's own design (a plain class
// constructed/updated/closed by the caller, e.g. SequenceEcho.cs).
//
// Typical usage, matching SequenceEcho.cs's per-frame wiring:
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

    // Called from update() with (data, num_bytes) once per poll that finds
    // new samples - mirrors ALCaptureDeviceSettings.DataCallback in the C#
    // reference, except this reports a byte count rather than a frame count
    // so callers (e.g. VAStreamSource::push_audio_data) never need to know
    // this device's bytes-per-frame themselves. data points into
    // sample_buffer, valid only for the duration of the callback (matches
    // ALCaptureDevice.cs's DataCallback, which is handed the same reused
    // native buffer each call).
    std::function<void(const uint8_t *, int)> data_callback;

    bool capturing = false;

public:
    ALCaptureDevice() = default;
    ~ALCaptureDevice();

    ALCaptureDevice(const ALCaptureDevice &) = delete;
    ALCaptureDevice &operator=(const ALCaptureDevice &) = delete;

    // Opens device_name (empty for the driver's default capture device - see
    // ALManager::get_available_capture_devices() for valid names) for
    // capturing at the given format/frequency, with an internal ring buffer
    // sized for buffer_size_frames frames. callback is invoked from update()
    // whenever new samples are available. Returns false (with a Godot error
    // already logged) on failure - is_valid() stays false in that case. Only
    // AL_FORMAT_MONO8/MONO16/STEREO8/STEREO16 are supported (the formats a
    // microphone realistically captures in) - see bytes_per_frame_for_format().
    bool open(const String &device_name, ALenum format, int frequency, int buffer_size_frames,
        std::function<void(const uint8_t *, int)> callback);

    // Starts the device capturing into its internal ring buffer. update()
    // must still be called each frame to actually pull samples out and reach
    // data_callback - matches alcCaptureStart's own "starts filling the ring
    // buffer in the background" semantics.
    void start();

    // Stops capturing. Already-buffered samples remain available to
    // update()/alcCaptureSamples until read.
    void stop();

    // Reads however many frames are currently available (capped at
    // buffer_size_frames) via alcCaptureSamples and, if any were read,
    // invokes data_callback with the resulting byte count. Call once per
    // frame - matches ALCaptureDevice.cs's Update(), called once per frame by
    // SequenceEcho.cs. No-ops if this device isn't open.
    void update();

    // Stops capturing (if still active) and closes the device. Safe to call
    // more than once and safe to call even if open() was never called or
    // failed. Also called by the destructor.
    void close();

    bool is_valid() const
    {
        return device != nullptr;
    }
};
