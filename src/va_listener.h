#pragma once

#include "va_emitter.h"

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace va_godot
{

// Port of vaudio-unreal's AVAudioListener: a purpose-named node for the world's single listener, rather than requiring users
// to add a plain VAEmitter and tick its is_main_listener checkbox themselves. A VAEmitter subclass that reuses its create/
// destroy and raytracing plumbing entirely, only forcing is_main_listener=true (hidden from the inspector via _validate_property).
class VAListener : public VAEmitter
{
    GDCLASS(VAListener, VAEmitter);

protected:
    static void _bind_methods();

public:
    VAListener();

    void _validate_property(PropertyInfo &p_property) const;
};

} // namespace va_godot
