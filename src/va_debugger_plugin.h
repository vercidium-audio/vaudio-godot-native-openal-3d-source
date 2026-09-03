#pragma once

#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

namespace va_godot
{

class VADebuggerPlugin : public EditorDebuggerPlugin
{
    GDCLASS(VADebuggerPlugin, EditorDebuggerPlugin);

protected:
    static void _bind_methods();

public:
    void sync_primitive(const String &scene_root_name, const NodePath &node_path, const String &material,
        const Variant &use_flat_transmission, const String &propagate);

    void sync_material_properties(const String &scene_root_name, const NodePath &node_path, const String &node_name,
        bool is_custom_material, int material_type, const String &custom_material_name, float absorption_lf,
        float absorption_hf, float scattering, float transmission_lf, float transmission_hf,
        float flat_transmission_lf, float flat_transmission_hf, const Color &color);

    // Relays the editor's viewport camera transform/FOV to every active game session, polled every frame by VAWorld's sync_viewport property via Engine::get_singleton (see VAConversionPlugin::_enter_tree for why this is a singleton, not pushed like the plugins above).
    void sync_viewport_camera(const Vector3 &position, const Vector3 &rotation, float fov_degrees);
};

} // namespace va_godot
