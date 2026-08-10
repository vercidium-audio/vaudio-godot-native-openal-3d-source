#include "va_world_gizmo.h"

#include <algorithm>
#include <cmath>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include "va_world.h"

namespace va_godot
{

// Handle IDs identify which face is being dragged - the handle sits at the midpoint of that
// face. Dragging a MAX_* handle only changes bounds_size along that axis; dragging a MIN_*
// handle moves bounds_position along that axis too, so the opposite (fixed) face stays put in
// world space.
enum VAWorldBoundsHandle
{
    VA_WORLD_BOUNDS_HANDLE_MAX_X,
    VA_WORLD_BOUNDS_HANDLE_MAX_Y,
    VA_WORLD_BOUNDS_HANDLE_MAX_Z,
    VA_WORLD_BOUNDS_HANDLE_MIN_X,
    VA_WORLD_BOUNDS_HANDLE_MIN_Y,
    VA_WORLD_BOUNDS_HANDLE_MIN_Z,
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

    // One drag handle per axis per side, at the midpoint of that face, so each handle only ever
    // resizes bounds_size (and, for the -face handles, bounds_position) along its own axis.
    PackedVector3Array handles;
    handles.push_back(Vector3(max.x, max.y * 0.5f, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, max.y, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, max.y * 0.5f, max.z));
    handles.push_back(Vector3(min.x, max.y * 0.5f, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, min.y, max.z * 0.5f));
    handles.push_back(Vector3(max.x * 0.5f, max.y * 0.5f, min.z));

    PackedInt32Array handle_ids;
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MAX_X);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MAX_Y);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MAX_Z);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MIN_X);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MIN_Y);
    handle_ids.push_back(VA_WORLD_BOUNDS_HANDLE_MIN_Z);

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
        case VA_WORLD_BOUNDS_HANDLE_MAX_X:
            return "Bounds Size +X";
        case VA_WORLD_BOUNDS_HANDLE_MAX_Y:
            return "Bounds Size +Y";
        case VA_WORLD_BOUNDS_HANDLE_MAX_Z:
            return "Bounds Size +Z";
        case VA_WORLD_BOUNDS_HANDLE_MIN_X:
            return "Bounds Size -X";
        case VA_WORLD_BOUNDS_HANDLE_MIN_Y:
            return "Bounds Size -Y";
        case VA_WORLD_BOUNDS_HANDLE_MIN_Z:
            return "Bounds Size -Z";
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
    // cancelled drag (e.g. Escape) can restore the exact pre-drag bounds_position/bounds_size -
    // both, since a -face drag changes both together. Note this is called on every _set_handle
    // frame (not just once at drag start) to refresh the editor's own cached restore value, so
    // drag-start detection must not live here - see _begin_handle_action.
    Array restore;
    restore.push_back(world->get_position());
    restore.push_back(world->get_bounds_size());
    return restore;
}

void VAWorldGizmoPlugin::_begin_handle_action(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary)
{
    // Fires exactly once per drag, at mouse-down, before the first _set_handle call - the
    // reliable place to reset drag_click_offset and snapshot drag_start_position/drag_start_size
    // for a fresh drag (see their header doc comments).
    drag_click_offset = NAN;

    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (world)
    {
        drag_start_position = world->get_position();
        drag_start_size = world->get_bounds_size();
    }
}

void VAWorldGizmoPlugin::_set_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, Camera3D *camera, const Vector2 &screen_pos)
{
    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return;

    Vector3::Axis axis;
    bool is_min_face;
    switch (handle_id)
    {
        case VA_WORLD_BOUNDS_HANDLE_MAX_X:
            axis = Vector3::AXIS_X;
            is_min_face = false;
            break;
        case VA_WORLD_BOUNDS_HANDLE_MAX_Y:
            axis = Vector3::AXIS_Y;
            is_min_face = false;
            break;
        case VA_WORLD_BOUNDS_HANDLE_MAX_Z:
            axis = Vector3::AXIS_Z;
            is_min_face = false;
            break;
        case VA_WORLD_BOUNDS_HANDLE_MIN_X:
            axis = Vector3::AXIS_X;
            is_min_face = true;
            break;
        case VA_WORLD_BOUNDS_HANDLE_MIN_Y:
            axis = Vector3::AXIS_Y;
            is_min_face = true;
            break;
        case VA_WORLD_BOUNDS_HANDLE_MIN_Z:
            axis = Vector3::AXIS_Z;
            is_min_face = true;
            break;
        default:
            return;
    }

    // Drag is constrained to the world-space line through the box's pre-drag min corner along
    // the chosen axis. Anchored to drag_start_position (rather than world->get_global_transform(),
    // which is live) because a -face drag moves bounds_position every frame - if the ray math
    // re-read the live origin, the axis line itself would shift out from under the drag each
    // frame instead of staying fixed for its duration.
    //
    // Cast the mouse position into a 3D ray and intersect it with a *plane* through the handle's
    // pre-drag world position - not the closest point on the ray to the axis *line* (what this
    // used to do, mirroring Gizmo3DHelper::box_set_handle at a glance). That line-based approach
    // is only exact when the handle sits on the axis itself (true for CSGBox3D/CollisionShape3D,
    // whose extent handles are at e.g. (size.x/2, 0, 0)) - ours sit at face centres, off-axis
    // (e.g. (max.x, max.y * 0.5, max.z * 0.5)), and for an off-axis handle the
    // closest-point-on-line solution advances faster than the cursor's actual on-screen motion
    // (verified numerically: a cursor moving a world-space delta of 2 units produced a
    // closest-point delta of 2.39, growing with distance from the axis and view angle) - this was
    // the "outruns the cursor" bug. Intersecting a camera-facing plane through the handle's actual
    // position instead makes the hit point track the cursor exactly 1:1 regardless of how far the
    // handle sits from the axis, because the ray always lands exactly where the cursor is
    // pointing on that plane.
    Node3D *parent = world->get_parent_node_3d();
    Transform3D parent_transform = parent ? parent->get_global_transform() : Transform3D();
    Vector3 axis_origin = parent_transform.xform(drag_start_position);
    Basis parent_basis = parent_transform.get_basis();
    Vector3 axis_direction = parent_basis.get_column(axis).normalized();

    Vector3 handle_offset = drag_start_size;
    handle_offset[axis] = is_min_face ? 0.0f : drag_start_size[axis];
    Vector3 handle_position = axis_origin + parent_basis.xform(handle_offset * 0.5f);

    Vector3 ray_origin = camera->project_ray_origin(screen_pos);
    Vector3 ray_direction = camera->project_ray_normal(screen_pos);

    // Plane contains the axis line and is tilted to face the camera as much as possible while
    // still containing that line - the standard construction for a view-aligned axis-constrained
    // drag plane (used by e.g. Blender/Unity's axis gizmos).
    Vector3 view_direction = handle_position - camera->get_global_transform().get_origin();
    Vector3 plane_normal = axis_direction.cross(view_direction.cross(axis_direction));
    float plane_normal_length = plane_normal.length();

    if (plane_normal_length < 0.0001f)
        return;

    plane_normal /= plane_normal_length;

    // Camera looking straight down the axis - the plane is edge-on to the ray, so moving the
    // mouse can't meaningfully change the position along the axis. Leave bounds_size/position
    // untouched this frame rather than dividing by ~zero.
    float ray_dot_plane = ray_direction.dot(plane_normal);
    if (std::abs(ray_dot_plane) < 0.0001f)
        return;

    float t = (handle_position - ray_origin).dot(plane_normal) / ray_dot_plane;
    Vector3 hit_point = ray_origin + ray_direction * t;

    // new_length is world-space distance from drag_start_position along the axis. Note this makes
    // the drag speed vary with distance from the camera (the face sweeps more world-space per
    // on-screen pixel the further it recedes along the view direction) - that's expected
    // perspective foreshortening inherent to any true 3D-space axis drag, not a bug.
    float new_length = (hit_point - axis_origin).dot(axis_direction);

    // Preserve the offset between where the user clicked and the handle's exact position (rather
    // than snapping the box edge to exactly under the cursor), so the drag starts smoothly from
    // the current face position instead of jumping - see drag_click_offset's doc comment. For a
    // +face handle the pre-drag face position (in the same "distance from drag_start_position"
    // units as new_length) is bounds_size[axis]; for a -face handle it's 0, since the min face
    // starts exactly at drag_start_position.
    if (std::isnan(drag_click_offset))
    {
        float start_face_length = is_min_face ? 0.0f : drag_start_size[axis];
        drag_click_offset = new_length - start_face_length;
    }

    float dragged_length = new_length - drag_click_offset;

    if (is_min_face)
    {
        // The min face moves to track the cursor and can grow outward without limit - the only
        // constraint is it can't cross the fixed max face, which would make bounds_size
        // negative. new_min is measured in parent-space units along the axis (same assumption
        // bounds_size itself already makes elsewhere in this file: no parent scale).
        float max_face_length = drag_start_size[axis];
        float new_min = std::min(dragged_length, max_face_length);

        Vector3 position = drag_start_position;
        position[axis] += new_min;
        world->set_position(position);

        Vector3 bounds_size = world->get_bounds_size();
        bounds_size[axis] = max_face_length - new_min;
        world->set_bounds_size(bounds_size);
    }
    else
    {
        Vector3 bounds_size = world->get_bounds_size();
        bounds_size[axis] = std::max(dragged_length, 0.0f);
        world->set_bounds_size(bounds_size);
    }
}

void VAWorldGizmoPlugin::_commit_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, const Variant &restore, bool cancel)
{
    VAWorld *world = Object::cast_to<VAWorld>(gizmo->get_node_3d());
    if (!world)
        return;

    // restore is [position, bounds_size] as snapshotted by _get_handle_value - both are needed
    // since a -face drag changes both together.
    Array restore_array = restore;
    Vector3 restore_position = restore_array[0];
    Vector3 restore_size = restore_array[1];

    if (cancel)
    {
        world->set_position(restore_position);
        world->set_bounds_size(restore_size);
        return;
    }

    // bounds_position/bounds_size already hold the final dragged values (set continuously by
    // _set_handle) - so register the UndoRedo action without re-executing the "do" step
    // (commit_action(false)), matching Godot's own built-in gizmos (e.g. GPUParticles3D's
    // extents handles).
    Vector3 final_position = world->get_position();
    Vector3 final_size = world->get_bounds_size();
    if (final_position == restore_position && final_size == restore_size)
        return;

    EditorUndoRedoManager *undo_redo = EditorInterface::get_singleton()->get_editor_undo_redo();
    undo_redo->create_action("Resize VAWorld");
    undo_redo->add_do_property(world, "position", final_position);
    undo_redo->add_do_property(world, "bounds_size", final_size);
    undo_redo->add_undo_property(world, "position", restore_position);
    undo_redo->add_undo_property(world, "bounds_size", restore_size);
    undo_redo->commit_action(false);
}

} // namespace va_godot
