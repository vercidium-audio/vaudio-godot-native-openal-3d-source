#pragma once

#include "va_emitter.h"

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace va_godot
{

// Port of vaudio-unreal's AVAudioListener: a purpose-named node for the
// world's single listener, rather than requiring users to add a plain
// VAEmitter and tick its is_main_listener checkbox themselves. Place exactly
// one of these under a VAWorld - every other VAEmitter/VASource is
// automatically added as one of its raytracing targets (see
// VAWorld::register_emitter).
//
// A subclass of VAEmitter rather than a from-scratch node (see
// native_godot_plan.md discussion) - it reuses VAEmitter's create/destroy,
// listener-registration, and per-frame raytracing-result plumbing entirely,
// and only forces is_main_listener=true (hidden from the inspector via
// _validate_property, since it's not meaningful to toggle off on this class).
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
