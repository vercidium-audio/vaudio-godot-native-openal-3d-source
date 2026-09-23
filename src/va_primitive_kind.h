#pragma once

// Which concrete ::VAXxxPrimitive type is stored in VAPrimitiveRef::primitive - needed since the C SDK's remove/destroy calls are typed, with no single untyped entry point.
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
