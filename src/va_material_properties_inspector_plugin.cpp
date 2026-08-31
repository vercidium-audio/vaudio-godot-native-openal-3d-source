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

void VAMaterialPropertiesInspectorPlugin::on_property_edited(const String &property_name)
{
    if (!current_node)
        return;

    if (!Object::cast_to<VADefaultMaterial>(current_node) && !Object::cast_to<VACustomMaterial>(current_node))
        return;

    sync_running_game(current_node);
}

void VAMaterialPropertiesInspectorPlugin::sync_running_game(Node *node)
{
    if (!debugger_plugin.is_valid())
        return;

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
        return;

    bool is_custom_material = Object::cast_to<VACustomMaterial>(node) != nullptr;

    // VADefaultMaterial has no material_name property and VACustomMaterial has no material_type property - only the
    // one that applies to this node's actual type is meaningful, the receiving end ignores the other.
    int material_type = is_custom_material ? 0 : (int)node->get("material_type");
    String custom_material_name = is_custom_material ? (String)node->get("material_name") : String();

    NodePath node_path = scene_root->get_path_to(node);

    float absorption_lf = node->get("absorption_lf");
    float absorption_hf = node->get("absorption_hf");
    float scattering = node->get("scattering");
    float transmission_lf = node->get("transmission_lf");
    float transmission_hf = node->get("transmission_hf");
    float flat_transmission_lf = node->get("flat_transmission_lf");
    float flat_transmission_hf = node->get("flat_transmission_hf");
    Color color = node->get("color");

    debugger_plugin->sync_material_properties(scene_root->get_name(), node_path, String(node->get_name()),
        is_custom_material, material_type, custom_material_name, absorption_lf, absorption_hf,
        scattering, transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
}
