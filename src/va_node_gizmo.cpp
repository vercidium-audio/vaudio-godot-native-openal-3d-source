#include "va_node_gizmo.h"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/triangle_mesh.hpp>

#include "va_emitter.h"
#include "va_raytraced_source.h"

namespace va_godot
{

// Brand green, matching the plugin's icons.
static const Color VA_NODE_GIZMO_COLOR = Color(0.52f, 1.0f, 0.64f);

static constexpr float VA_NODE_GIZMO_RADIUS = 0.5f;

void VANodeGizmoPlugin::_bind_methods()
{
}

VANodeGizmoPlugin::VANodeGizmoPlugin()
{
    sphere_mesh.instantiate();
    sphere_mesh->set_radius(VA_NODE_GIZMO_RADIUS);
    sphere_mesh->set_height(VA_NODE_GIZMO_RADIUS * 2.0f);
    sphere_mesh->set_radial_segments(32);
    sphere_mesh->set_rings(16);

    // Fully lit (SHADING_MODE_PER_PIXEL, the default) and opaque, so it reads as a real solid object: it takes the scene's lighting and occludes geometry behind it. A little emission keeps it visible in an unlit / dark scene.
    Ref<StandardMaterial3D> fill;
    fill.instantiate();
    fill->set_albedo(VA_NODE_GIZMO_COLOR);
    fill->set_roughness(0.55f);
    fill->set_metallic(0.0f);
    fill->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
    fill->set_emission(VA_NODE_GIZMO_COLOR);
    fill->set_emission_energy_multiplier(0.35f);
    add_material("va_node_fill", fill);
}

bool VANodeGizmoPlugin::_has_gizmo(Node3D *for_node_3d) const
{
    // VAListener is a VAEmitter subclass; VASource/VAStreamSource (and VAInputStreamSource/VANetworkedStreamSource) all derive from VARaytracedSource - so these two checks catch all four requested node types and anything derived from them.
    return Object::cast_to<VAEmitter>(for_node_3d) != nullptr || Object::cast_to<VARaytracedSource>(for_node_3d) != nullptr;
}

String VANodeGizmoPlugin::_get_gizmo_name() const
{
    return "VANode";
}

void VANodeGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo> &gizmo)
{
    gizmo->clear();
    gizmo->add_mesh(sphere_mesh, get_material("va_node_fill", gizmo));

    // add_mesh is visual only - the editor won't click-select a node by its gizmo unless the gizmo also contributes collision geometry the editor can raycast against. SphereMesh builds its own TriangleMesh (same faces as the drawn mesh), which is exactly what add_collision_triangles wants.
    Ref<TriangleMesh> tri_mesh = sphere_mesh->generate_triangle_mesh();
    if (tri_mesh.is_valid())
        gizmo->add_collision_triangles(tri_mesh);
}

} // namespace va_godot
