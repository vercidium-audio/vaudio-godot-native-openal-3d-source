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
}

// Hides the inherited is_main_listener property from the inspector - it's
// always true on this class (see the constructor) and toggling it off would
// leave a VAListener node that isn't actually the listener, which makes no
// sense given the class's whole purpose.
void VAListener::_validate_property(PropertyInfo &p_property) const
{
    if (p_property.name == StringName("is_main_listener"))
    {
        p_property.usage = PROPERTY_USAGE_NONE;
    }
}

} // namespace va_godot
