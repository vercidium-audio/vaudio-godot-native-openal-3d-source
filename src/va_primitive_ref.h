#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

extern "C"
{
#include <vaudio.h>
}

#include "transform_watcher.h"

using namespace godot;

// Which concrete ::VAXxxPrimitive type is stored in VAPrimitiveRef::primitive.
// Needed because vaWorldRemovePrimitive_/the per-type Destroy calls are typed
// in the C SDK - there is no single untyped "remove"/"destroy" entry point.
enum class VAPrimitiveKind
{
    Prism,
    Cylinder,
    Cone,
    Sphere,
    Capsule,
    Plane,
    Mesh,
};

// Stashed as Node metadata (see VAWorld::PRIMITIVE_META_KEY) so a primitive can
// be removed/updated later when its owning node moves or leaves the scene tree.
// Mirrors vaudio-godot-openal's VAPrimitiveRef (misc/VAPrimitiveRef.cs).
class VAPrimitiveRef : public RefCounted
{
    GDCLASS(VAPrimitiveRef, RefCounted);

protected:
    static void _bind_methods()
    {
    }

public:
    void *primitive = nullptr;
    VAPrimitiveKind kind = VAPrimitiveKind::Prism;
    TransformWatcher *watcher = nullptr;

    // Only set for CollisionShape3D nodes whose Shape3D resource can itself
    // change (e.g. BoxShape3D's "changed" signal) - not currently connected
    // to anything in this port, kept for parity with the C# field.
    Ref<Resource> shape_resource;
};
