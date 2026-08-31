#pragma once

#include "va_emitter.h"

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace va_godot
{

class VAListener : public VAEmitter
{
    GDCLASS(VAListener, VAEmitter);

protected:
    static void _bind_methods();

public:
    VAListener();

    bool is_main_listener() const override
    {
        return true;
    }

    void _validate_property(PropertyInfo &p_property) const;
};

} // namespace va_godot
