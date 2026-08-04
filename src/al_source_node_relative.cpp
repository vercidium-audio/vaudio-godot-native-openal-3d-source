#include "al_source_node_relative.h"

#include "openal/al_source.h"

void ALSourceNodeRelative::_bind_methods()
{
}

ALSourceNodeRelative::ALSourceNodeRelative()
{
}

ALSourceNodeRelative::~ALSourceNodeRelative()
{
}

void ALSourceNodeRelative::configure_source(ALSource &source)
{
    // Relative to the listener with zero offset
    source.set_relative(true);
    source.set_position(Vector3());
}
