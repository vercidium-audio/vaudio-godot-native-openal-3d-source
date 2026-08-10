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

// Draws a wireframe box in the 3D viewport showing VAWorld's bounds_position/bounds_size AABB,
// with draggable handles on all six faces so bounds_size can be resized directly in the
// viewport. Dragging a +face handle only changes bounds_size (the opposite -face stays put);
// dragging a -face handle moves bounds_position on that axis while shrinking/growing bounds_size
// to compensate, so the opposite +face stays fixed in world space.
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

    // Offset between where the user clicked and the handle's exact on-axis position, so a drag
    // doesn't snap the box edge to exactly under the cursor - only the cursor's movement from
    // where the drag began is tracked. Reset once per drag by _begin_handle_action (which fires
    // exactly once at mouse-down, unlike _get_handle_value, which the editor also re-queries on
    // every _set_handle frame to refresh its own cached restore value). NaN sentinels "no drag in
    // progress yet"; only one drag can be active at a time, so plain instance state is
    // sufficient.
    float drag_click_offset = NAN;

    // bounds_position/bounds_size as of the start of the current drag (see
    // _begin_handle_action). A -face drag moves bounds_position, which would otherwise move the
    // ray-cast axis origin mid-drag if it were read from the node's live global_transform each
    // frame - anchoring both the ray-cast axis and the fixed opposite face to their pre-drag
    // values keeps a single drag's math self-consistent frame to frame, the same way
    // drag_click_offset anchors the click-to-handle offset.
    Vector3 drag_start_position;
    Vector3 drag_start_size;
};

} // namespace va_godot
