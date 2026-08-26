#pragma once

#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace va_godot
{

// Relays "Vercidium Audio" material/permeation edits made in the Inspector to the running game's own process - EditorInspectorPlugin
// controls (see VAMaterialInspectorPlugin) only run against the editor's local copy of the scene, whose VAWorld has no live raytracing
// world. Godot's debugger protocol is the only bridge, so this sends a "vaudio:sync_primitive" message to every active session; the
// running game receives it via an EngineDebugger message capture registered in register_types.cpp.
class VADebuggerPlugin : public EditorDebuggerPlugin
{
    GDCLASS(VADebuggerPlugin, EditorDebuggerPlugin);

protected:
    static void _bind_methods();

public:
    // node_path is relative to the edited scene's root (scene_root_name) - SceneTree::current_scene isn't reliable (a running game may
    // add a scene as a plain child, e.g. car_select.gd never updates it), so the receiving end searches for scene_root_name anywhere
    // under the running game's root instead (see on_debugger_message in register_types.cpp). material/use_flat_transmission carry the
    // edited metadata across since the running game's copy of this node wasn't touched by the edit; empty material means Air (no
    // metadata, matching remove_meta in VAMaterialInspectorPlugin::on_material_selected), NIL use_flat_transmission means "use default".
    void sync_primitive(const String &scene_root_name, const NodePath &node_path, const String &material, const Variant &use_flat_transmission);

    // Relays a VADefaultMaterial/VACustomMaterial property edit made in the Inspector while the game is running - see
    // VAMaterialPropertiesInspectorPlugin. Received by on_debugger_message in register_types.cpp, which applies the
    // values directly via apply_properties_from_editor - unlike sync_primitive above, there's no metadata to carry
    // across and no primitive to re-add.
    void sync_material_properties(const String &scene_root_name, const NodePath &node_path, float absorption_lf,
        float absorption_hf, float scattering, float transmission_lf, float transmission_hf,
        float flat_transmission_lf, float flat_transmission_hf, const Color &color);
};

} // namespace va_godot
