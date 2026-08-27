#pragma once

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

using namespace godot;

namespace va_godot
{

// Draws a solid shaded sphere in the editor viewport at every VAEmitter/VAListener/VASource/VAStreamSource (and their subclasses), so they're visible while editing a scene even though they have no mesh of their own. Editor-only - gizmos never render in the running game.
class VANodeGizmoPlugin : public EditorNode3DGizmoPlugin
{
    GDCLASS(VANodeGizmoPlugin, EditorNode3DGizmoPlugin);

protected:
    static void _bind_methods();

public:
    VANodeGizmoPlugin();

    bool _has_gizmo(Node3D *for_node_3d) const override;
    String _get_gizmo_name() const override;
    void _redraw(const Ref<EditorNode3DGizmo> &gizmo) override;

private:
    // Built once in the constructor and reused for every redraw - a fixed-size SphereMesh has no per-node state.
    Ref<SphereMesh> sphere_mesh;
};

} // namespace va_godot
