#include "va_listener.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace va_godot
{

void VAListener::_bind_methods()
{
}

VAListener::VAListener()
{
    // Forces this node to always be the world's listener - matches
    // AVAudioListener always registering itself as the raytracing world's
    // single reference point, without needing a separate checkbox.
    set_is_main_listener(true);
    set_has_relative_reverb(true);
}

// Hides inherited properties from the inspector that aren't meaningful on a
// listener: is_main_listener is always true (see the constructor), and
// affects_grouped_eax/occlusion_energy_cap/permeation_energy_cap only apply
// to emitters that can be occluded/permeated, not the listener itself.
void VAListener::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("is_main_listener") ||
        p_property.name == StringName("affects_grouped_eax") ||
        p_property.name == StringName("occlusion_energy_cap") ||
        p_property.name == StringName("permeation_energy_cap"))
    {
        p_property.usage = PROPERTY_USAGE_NONE;
    }
}

} // namespace va_godot
