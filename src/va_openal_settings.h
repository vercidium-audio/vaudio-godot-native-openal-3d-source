#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "openal/al_functions.h"
#include "va_device_name.h"

using namespace godot;

namespace va_godot
{

// Global OpenAL device/mixer settings node - the native C++ equivalent of
// vaudio-godot-mono-openal-3d's ALManager autoload (see todo.md item 1). Unlike
// per-VAWorld settings (VAWorld's own exported properties), master volume,
// output device, and the auxiliary-send count are properties of the single
// process-wide OpenAL device (ALManager), so they live here instead of on
// VAWorld - add exactly one of these anywhere in the scene tree (an autoload
// is the natural place, matching the C# reference).
//
// _ready() (not _enter_tree()) pushes these into ALManager deliberately -
// ALManager::initialize() already opened a default device at module load
// (register_types.cpp), before this node's _enter_tree could run for a
// scene-tree-order-dependent reason; _ready() firing after the whole initial
// scene has entered the tree keeps this node's own position in the scene
// irrelevant to whether its settings "win".
class VAOpenALSettings : public Node
{
    GDCLASS(VAOpenALSettings, Node);

private:
    // Empty means "use the driver's default playback device", matching
    // ALManager's own default - this is the internal/normalized form
    // consumed directly by ALManager::reinitialize. get_device_name() maps
    // "" to DEFAULT_DEVICE_LABEL and set_device_name() maps it back, so the
    // Node's own property getter (what the inspector's enum dropdown reads
    // for its "current value") never returns "" - otherwise that dropdown
    // shows a second, unlabelled blank entry alongside the labelled one in
    // hint_string. Matched against ALManager::get_available_devices() by
    // name (not index) - device lists can reorder between driver sessions,
    // but names are stable.
    String device_name;

    // Same "" = default convention as device_name above, with the same
    // get/set_capture_device_name DEFAULT_DEVICE_LABEL translation for the
    // inspector's benefit. Unlike device_name above, this isn't pushed
    // anywhere automatically by this class - VAOpenALSettings only owns the
    // playback device/context (ALManager). Capture devices are opened
    // independently by whatever script/node creates an ALCaptureDevice (see
    // openal/al_capture_device.h); this property exists purely as an
    // inspector-discoverable name to pass to ALCaptureDevice::open(),
    // matching godot_stream_plan.md section 6's "device selection... matched
    // to the existing get_available_devices()/refresh_devices() pattern".
    String capture_device_name;

    // 0 means "leave ALC_MAX_AUXILIARY_SENDS at the driver default" - see
    // ALManager::open_device_and_context.
    int max_reverb_sends = 0;

    float master_volume = 1.0f;

    // 0 means "leave ALC_FREQUENCY (the mixing sample rate) at the driver
    // default" - see ALManager::open_device_and_context.
    int sample_rate = 0;

    // Requests HRTF-based binaural rendering via the ALC_SOFT_HRTF extension
    // instead of the driver's default panning. The driver can still refuse
    // (e.g. no HRTF data for the output device).
    bool hrtf_enabled = false;

    // AL distance model, exposed as a PROPERTY_HINT_ENUM over OpenAL's
    // handful of AL_*_DISTANCE(_CLAMPED) constants - see get/set_distance_model.
    int distance_model = AL_INVERSE_DISTANCE_CLAMPED;

    // AL_METERS_PER_UNIT (EFX) listener property - the OpenAL/EFX-side scale
    // used by air-absorption and reverb decay math. Distinct from
    // VAWorld::meters_per_unit, which only feeds vaudio's own raytracing/
    // acoustics model - the two aren't linked and can be set independently.
    float meters_per_unit = 1.0f;

    // Global AL speed of sound (alSpeedOfSound), used by OpenAL's own Doppler
    // calculation. Distinct from VAWorld::speed_of_sound, which only feeds
    // vaudio's raytracing/acoustics model.
    float speed_of_sound = 343.0f;

    // When true, every source's dry/direct path is silenced (routed through
    // a gain-0 filter) so only reverb sends are audible - a diagnostic
    // toggle for tuning reverb in isolation. See ALManager::reverb_only.
    bool reverb_only = false;

protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &property) const;

public:
    void _ready() override;

    String get_device_name() const;
    void set_device_name(const String &value);

    String get_capture_device_name() const;
    void set_capture_device_name(const String &value);

    int get_max_reverb_sends() const;
    void set_max_reverb_sends(int value);

    float get_master_volume() const;
    void set_master_volume(float value);

    int get_sample_rate() const;
    void set_sample_rate(int value);

    bool get_hrtf_enabled() const;
    void set_hrtf_enabled(bool value);

    int get_distance_model() const;
    void set_distance_model(int value);

    float get_meters_per_unit() const;
    void set_meters_per_unit(float value);

    float get_speed_of_sound() const;
    void set_speed_of_sound(float value);

    bool get_reverb_only() const;
    void set_reverb_only(bool value);

    // Callable from the editor/GDScript (e.g. via the Inspector's method
    // list or a debug script) to discover valid device_name values - not
    // surfaced as a property hint since the driver's device list is only
    // known at runtime, after ALManager has loaded soft_oal.dll.
    PackedStringArray get_available_devices() const;

    // Same as get_available_devices(), but for capture (microphone/input)
    // devices - names discovered here are what ALCaptureDevice::open()'s
    // device_name parameter accepts.
    PackedStringArray get_available_capture_devices() const;

    // Re-queries the driver's device list and re-validates device_name's
    // PROPERTY_HINT_ENUM so a device plugged in after this node was selected
    // in the editor shows up without reselecting the node or restarting the
    // editor. Wired to the "Refresh Devices" button
    // VADeviceRefreshInspectorPlugin adds in the Inspector (va_conversion_plugin.cpp) -
    // Godot only rebuilds a cached property list on specific triggers
    // (selection, this call, scene reload), never by polling for hardware
    // changes on its own.
    void refresh_devices();
};

} // namespace va_godot
