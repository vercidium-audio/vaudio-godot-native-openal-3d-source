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
// with draggable handles on the +X/+Y/+Z faces so bounds_size can be resized directly in the
// viewport (the min corner is fixed at the node's own position, see VAWorld::set_position).
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
    void _set_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, Camera3D *camera, const Vector2 &screen_pos) override;
    void _commit_handle(const Ref<EditorNode3DGizmo> &gizmo, int32_t handle_id, bool secondary, const Variant &restore, bool cancel) override;

private:
    static Ref<ArrayMesh> build_face_mesh(const Vector3 corners[8]);
};

} // namespace va_godot
