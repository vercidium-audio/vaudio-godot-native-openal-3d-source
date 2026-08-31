#include "al_source_relative.h"

#include "openal/al_source_handle.h"

void ALSourceRelative::_bind_methods()
{
}

ALSourceRelative::ALSourceRelative()
{
}

ALSourceRelative::~ALSourceRelative()
{
}

void ALSourceRelative::configure_source(ALSourceHandle &source)
{
    // Relative to the listener with zero offset
    source.set_relative(true);
    source.set_position(Vector3());
}
