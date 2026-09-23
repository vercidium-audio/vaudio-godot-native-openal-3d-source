#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

extern "C"
{
#include <vaudio.h>
}

#include "transform_watcher.h"
#include "va_primitive_kind.h"

using namespace godot;

// Stashed as Node metadata (see VAWorld::PRIMITIVE_META_KEY) so a primitive can be removed/updated later when its owning node moves or leaves the scene tree.
class VAPrimitiveRef : public RefCounted
{
    GDCLASS(VAPrimitiveRef, RefCounted);

protected:
    static void _bind_methods()
    {
    }

public:
    void *primitive = nullptr;
    VAPrimitiveKind kind; // always set explicitly at construction, no sensible dimension-neutral default
    TransformWatcher *watcher = nullptr;

    // Only set for CollisionShape nodes whose Shape resource can itself change; not currently wired up, kept for parity with the C# field.
    Ref<Resource> shape_resource;
};
