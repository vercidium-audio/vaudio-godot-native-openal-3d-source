#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace va_godot
{

// Global OpenAL device/mixer settings node - the native C++ equivalent of
// vaudio-godot-openal's ALManager autoload (see todo.md item 1). Unlike
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
    // ALManager's own default. Matched against ALManager::get_available_devices()
    // by name (not index) - device lists can reorder between driver
    // sessions, but names are stable.
    String device_name;

    // 0 means "leave ALC_MAX_AUXILIARY_SENDS at the driver default" - see
    // ALManager::open_device_and_context.
    int max_reverb_sends = 0;

    float master_volume = 1.0f;

protected:
    static void _bind_methods();

public:
    void _ready() override;

    String get_device_name() const;
    void set_device_name(const String &value);

    int get_max_reverb_sends() const;
    void set_max_reverb_sends(int value);

    float get_master_volume() const;
    void set_master_volume(float value);

    // Callable from the editor/GDScript (e.g. via the Inspector's method
    // list or a debug script) to discover valid device_name values - not
    // surfaced as a property hint since the driver's device list is only
    // known at runtime, after ALManager has loaded soft_oal.dll.
    PackedStringArray get_available_devices() const;
};

} // namespace va_godot
