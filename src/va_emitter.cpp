#include "va_emitter.h"

// The rest of VAEmitter is dimension-agnostic and lives in common/va_emitter.cpp.

namespace va_godot
{

Vector3 VAEmitter::get_va_position() const
{
    VAVector position = vaEmitterGetPosition(get_handle());
    return Vector3(position.x, position.y, position.z);
}

} // namespace va_godot
