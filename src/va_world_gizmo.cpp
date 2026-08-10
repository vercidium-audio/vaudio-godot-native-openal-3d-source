#include "va_world_gizmo.h"

#include <algorithm>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/geometry3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include "va_world.h"

namespace va_godot
{

// Handle IDs identify which axis of bounds_size is being dragged - the handle sits at the
// midpoint of the +X/+Y/+Z face of the box, so dragging it only ever changes that one axis.
enum VAWorldBoundsHandle
{
    VA_WORLD_BOUNDS_HANDLE_X,
    VA_WORLD_BOUNDS_HANDLE_Y,
    VA_WORLD_BOUNDS_HANDLE_Z,
};

void VAWorldGizmoPlugin::_bind_methods()
{
}

VAWorldGizmoPlugin::VAWorldGizmoPlugin()
{
    create_handle_material("handles");
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

    // Built per-instance (rather than via the plugin-wide create_material cache) since each
    // VAWorld can have its own user-set bounds_color.
    Color line_color = world->get_bounds_color();

    Ref<StandardMaterial3D> line_material;
    line_material.instantiate();
    line_material->set_albedo(line_color);
    line_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    gizmo->add_lines(lines, line_material);

    Color face_color = line_color;

    Ref<StandardMaterial3D> face_material;
    face_material.instantiate();
    face_material->set_albedo(face_color);
    face_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    face_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    face_material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    gizmo->add_mesh(build_face_mesh(corners), face_material);

    // One drag handle per axis, at the midpoint of that axis' +face, so each handle only ever
    // resizes bounds_size along its own axis.
    PackedVector3Array handles;
    handles.push_back(Vector3(max.x, max.y * 0.5f, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, max.y, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, max.y * 0.5f, max.z));

    PackedInt32Array handle_ids;
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_X);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_Y);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_Z);

    gizmo->add_handles(handles, get_material("handles", gizmo), handle_ids);
}

Ref<ArrayMesh> VAWorldGizmoPlugin::build_face_mesh(const Vector3 corners[8])
{
    int faces[6][4] = {
        { 0, 1, 2, 3 }, // bottom
        { 4, 7, 6, 5 }, // top
        { 0, 4, 5, 1 }, // -z side
        { 1, 5, 6, 2 }, // +x side
        { 2, 6, 7, 3 }, // +z side
        { 3, 7, 4, 0 }, // -x side
    };

    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    for (int i = 0; i < 6; i++)
    {
        Vector3 a = corners[faces[i][0]];
        Vector3 b = corners[faces[i][1]];
        Vector3 c = corners[faces[i][2]];
        Vector3 d = corners[faces[i][3]];

        st->add_vertex(a);
        st->add_vertex(b);
        st->add_vertex(c);

        st->add_vertex(a);
        st->add_vertex(c);
        st->add_vertex(d);
    }

    return st->commit();
}

String VAWorldGizmoPlugin::_get_handle_name(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary) const
{
    switch (handle_id)
    {
        case VA_WORLD_BOUNDS_HANDLE_X:
            return "Bounds Size X";
        case VA_WORLD_BOUNDS_HANDLE_Y:
            return "Bounds Size Y";
        case VA_WORLD_BOUNDS_HANDLE_Z:
            return "Bounds Size Z";
        default:
            return "";
    }
}

Variant VAWorldGizmoPlugin::_get_handle_value(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary) const
{
    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return Variant();

    // Snapshotted here and handed back to _commit_handle/_set_handle as p_restore, so a
    // cancelled drag (e.g. Escape) can restore the exact pre-drag bounds_size.
    return world->get_bounds_size();
}

void VAWorldGizmoPlugin::_set_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, Camera3D *camera, const Vector2 &screen_pos)
{
    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return;

    Vector3::Axis axis;
    switch (handle_id)
    {
        case VA_WORLD_BOUNDS_HANDLE_X:
            axis = Vector3::AXIS_X;
            break;
        case VA_WORLD_BOUNDS_HANDLE_Y:
            axis = Vector3::AXIS_Y;
            break;
        case VA_WORLD_BOUNDS_HANDLE_Z:
            axis = Vector3::AXIS_Z;
            break;
        default:
            return;
    }

    // Drag is constrained to the world-space line through the node's origin along the chosen
    // axis - find the point on that line closest to the mouse ray, mirroring how Godot's own
    // built-in box-shaped gizmos (e.g. GPUParticles3D's extents) implement axis-drag handles.
    Transform3D transform = world->get_global_transform();
    Vector3 axis_origin = transform.get_origin();
    Vector3 axis_direction = transform.get_basis().get_column(axis).normalized();

    Vector3 ray_origin = camera->project_ray_origin(screen_pos);
    Vector3 ray_direction = camera->project_ray_normal(screen_pos);

    PackedVector3Array closest_points = Geometry3D::get_singleton()->get_closest_points_between_segments(
        axis_origin - axis_direction * 4096.0f,
        axis_origin + axis_direction * 4096.0f,
        ray_origin,
        ray_origin + ray_direction * 4096.0f);

    if (closest_points.size() != 2)
        return;

    float new_length = (closest_points[0] - axis_origin).dot(axis_direction);

    Vector3 bounds_size = world->get_bounds_size();
    bounds_size[axis] = std::max(new_length, 0.0f);
    world->set_bounds_size(bounds_size);
}

void VAWorldGizmoPlugin::_commit_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, const Variant &restore, bool cancel)
{
    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return;

    if (cancel)
        world->set_bounds_size(restore);
}

} // namespace va_godot
