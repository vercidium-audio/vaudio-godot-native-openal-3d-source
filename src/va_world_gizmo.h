#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

namespace va_godot
{

// Draws a wireframe box showing VAWorld's bounds AABB with draggable resize handles on all six faces; a -face drag also moves bounds_position so the opposite +face stays fixed in world space.
class VAWorldGizmoPlugin : public EditorNode3DGizmoPlugin
{
    GDCLASS(VAWorldGizmoPlugin, EditorNode3DGizmoPlugin);

protected:
    static void _bind_methods();

public:
    VAWorldGizmoPlugin();

    bool _has_gizmo(Node3D *for_node_3d) const override;
    String _get_gizmo_name() const override;
    void _redraw(const Ref<EditorNode3DGizmo> &gizmo) override;

    String _get_handle_name(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary) const override;
    Variant _get_handle_value(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary) const override;
    void _begin_handle_action(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary) override;
    void _set_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, Camera3D *camera, const Vector2 &screen_pos) override;
    void _commit_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, const Variant &restore, bool cancel) override;

private:
    static Ref<ArrayMesh> build_face_mesh(const Vector3 corners[8]);

    // Offset between where the user clicked and the handle's exact position, so a drag doesn't snap the box edge to under the cursor. Reset once per drag by _begin_handle_action; NaN means no drag in progress.
    float drag_click_offset = NAN;

    // bounds_position/bounds_size as of the start of the current drag - anchoring to these (rather than the live, moving global_transform) keeps a single drag's math self-consistent frame to frame.
    Vector3 drag_start_position;
    Vector3 drag_start_size;
};

} // namespace va_godot
