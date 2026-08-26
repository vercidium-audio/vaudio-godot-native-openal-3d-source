#include "va_material_properties_inspector_plugin.h"

#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "va_custom_material.h"
#include "va_default_material.h"

using namespace va_godot;

void VAMaterialPropertiesInspectorPlugin::_bind_methods()
{
}

bool VAMaterialPropertiesInspectorPlugin::_can_handle(Object *object) const
{
    current_node = Object::cast_to<Node>(object);

    return Object::cast_to<VADefaultMaterial>(object) != nullptr || Object::cast_to<VACustomMaterial>(object) != nullptr;
}

void VAMaterialPropertiesInspectorPlugin::set_debugger_plugin(const Ref<VADebuggerPlugin> &plugin)
{
    debugger_plugin = plugin;

    EditorInspector *inspector = EditorInterface::get_singleton()->get_inspector();

    if (inspector && !inspector->is_connected("property_edited", callable_mp(this, &VAMaterialPropertiesInspectorPlugin::on_property_edited)))
        inspector->connect("property_edited", callable_mp(this, &VAMaterialPropertiesInspectorPlugin::on_property_edited));
}

// property_edited only reports the edited property's name, not which object it belongs to - current_node (set by
// _can_handle just before this plugin drew that object's properties) is used as a best-effort match. Fires for every
// Inspector edit in the editor, not just our own node types, so the _can_handle-style type check has to be repeated here.
void VAMaterialPropertiesInspectorPlugin::on_property_edited(const String &property_name)
{
    if (!current_node)
        return;

    if (!Object::cast_to<VADefaultMaterial>(current_node) && !Object::cast_to<VACustomMaterial>(current_node))
        return;

    sync_running_game(current_node);
}

// Mirrors VAMaterialInspectorPlugin::sync_running_game - see its header comment for why the debugger protocol is the
// only bridge between the editor's local copy of this node and the running game's separate process/copy.
void VAMaterialPropertiesInspectorPlugin::sync_running_game(Node *node)
{
    if (!debugger_plugin.is_valid())
        return;

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
        return;

    NodePath node_path = scene_root->get_path_to(node);

    float absorption_lf = node->get("absorption_lf");
    float absorption_hf = node->get("absorption_hf");
    float scattering = node->get("scattering");
    float transmission_lf = node->get("transmission_lf");
    float transmission_hf = node->get("transmission_hf");
    float flat_transmission_lf = node->get("flat_transmission_lf");
    float flat_transmission_hf = node->get("flat_transmission_hf");
    Color color = node->get("color");

    debugger_plugin->sync_material_properties(scene_root->get_name(), node_path, absorption_lf, absorption_hf,
        scattering, transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
}
