#include "va_world_gizmo.h"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include "va_world.h"

namespace va_godot
{

void VAWorldGizmoPlugin::_bind_methods()
{
}

VAWorldGizmoPlugin::VAWorldGizmoPlugin()
{
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
    face_color.a *= 0.15f;

    Ref<StandardMaterial3D> face_material;
    face_material.instantiate();
    face_material->set_albedo(face_color);
    face_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    face_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    face_material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    gizmo->add_mesh(build_face_mesh(corners), face_material);
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

} // namespace va_godot
