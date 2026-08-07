#include "va_world_gizmo.h"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include "va_world.h"

namespace va_godot
{

void VAWorldGizmoPlugin::_bind_methods()
{
}

VAWorldGizmoPlugin::VAWorldGizmoPlugin()
{
    create_material("va_world_bounds", Color(1.0f, 0.65f, 0.0f));
}

bool VAWorldGizmoPlugin::_has_gizmo(Node3D *for_node_3d) const
{
    return Object::cast_to<VAWorld>(for_node_3d) != nullptr;
}

String VAWorldGizmoPlugin::_get_gizmo_name() const
{
    return "VAWorld";
}

void VAWorldGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo> &gizmo)
{
    gizmo->clear();

    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return;

    // The node's own position is the AABB's world-space origin (see VAWorld::set_position);
    // rotation/scale are locked out (see VAWorld::_validate_property), so the box is simply
    // [0, bounds_size] in this node's local space.
    Vector3 min = Vector3();
    Vector3 max = world->get_bounds_size();

    Vector3 corners[8] = {
        Vector3(min.x, min.y, min.z),
        Vector3(max.x, min.y, min.z),
        Vector3(max.x, min.y, max.z),
        Vector3(min.x, min.y, max.z),
        Vector3(min.x, max.y, min.z),
        Vector3(max.x, max.y, min.z),
        Vector3(max.x, max.y, max.z),
        Vector3(min.x, max.y, max.z),
    };

    int edges[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, // bottom face
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, // top face
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }, // vertical edges
    };

    PackedVector3Array lines;

    for (int i = 0; i < 12; i++)
    {
        lines.push_back(corners[edges[i][0]]);
        lines.push_back(corners[edges[i][1]]);
    }

    Ref<Material> material = get_material("va_world_bounds", gizmo);
    gizmo->add_lines(lines, material);
}

} // namespace va_godot
