#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

namespace va_godot
{

// Draws a wireframe box in the 3D viewport showing VAWorld's bounds_position/bounds_size AABB.
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

private:
    static Ref<ArrayMesh> build_face_mesh(const Vector3 corners[8]);
};

} // namespace va_godot
