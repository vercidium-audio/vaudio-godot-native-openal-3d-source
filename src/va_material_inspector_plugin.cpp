#include "va_material_inspector_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "al_source_node.h"
#include "va_custom_material.h"
#include "va_emitter.h"
#include "va_world.h"

using namespace va_godot;

namespace
{

// "Air" (index 0) is deliberately omitted from the dropdown - it means "no vaudio geometry",
// which is what leaving the metadata unset already means, so there's no need for an explicit entry.
const int FIRST_SELECTABLE_BUILTIN_INDEX = 1;

// Custom materials only register their name with the running ::VAWorld at runtime
// (VACustomMaterial::_enter_tree), so at edit time this walks the scene tree directly instead.
void find_custom_materials_recursive(Node *node, PackedStringArray &out_names)
{
    if (VACustomMaterial *material = Object::cast_to<VACustomMaterial>(node))
        out_names.push_back(material->get_material_name());

    TypedArray<Node> children = node->get_children();

    for (int i = 0; i < children.size(); i++)
        find_custom_materials_recursive(Object::cast_to<Node>(children[i]), out_names);
}

} // namespace

void VAMaterialInspectorPlugin::_bind_methods()
{
}

bool VAMaterialInspectorPlugin::_can_handle(Object *object) const
{
    // VAWorld, VAEmitter/VAListener, and ALSourceNode/VASource* are Node3D too, but they're audio
    // control nodes, not geometry - a material dropdown on them would just be confusing noise.
    if (Object::cast_to<VAWorld>(object) || Object::cast_to<VAEmitter>(object) || Object::cast_to<ALSourceNode>(object))
        return false;

    return Object::cast_to<Node3D>(object) != nullptr;
}

void VAMaterialInspectorPlugin::_parse_end(Object *object)
{
    Node *node = Object::cast_to<Node>(object);
    if (!node)
        return;

    StringName material_meta_key = VAWorld::get_material_meta_key();

    VBoxContainer *container = memnew(VBoxContainer);

    Label *header = memnew(Label);
    header->set_text("Vercidium Audio");
    container->add_child(header);

    OptionButton *option_button = memnew(OptionButton);
    option_button->add_item("Air (no geometry)");

    PackedStringArray builtin_names = VAWorld::get_builtin_material_names();
    for (int i = FIRST_SELECTABLE_BUILTIN_INDEX; i < builtin_names.size(); i++)
        option_button->add_item(builtin_names[i]);

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    PackedStringArray custom_names;

    if (scene_root)
        find_custom_materials_recursive(scene_root, custom_names);

    if (!custom_names.is_empty())
    {
        option_button->add_separator("Custom Materials");

        for (int i = 0; i < custom_names.size(); i++)
            option_button->add_item(custom_names[i]);
    }

    String current_value = node->has_meta(material_meta_key) ? String(node->get_meta(material_meta_key)) : String();
    String current_lower = current_value.to_lower();
    int selected_index = 0;

    for (int i = 0; i < option_button->get_item_count(); i++)
    {
        if (option_button->is_item_separator(i))
            continue;

        if (option_button->get_item_text(i).to_lower() == current_lower)
        {
            selected_index = i;
            break;
        }
    }

    option_button->select(selected_index);
    option_button->connect("item_selected", callable_mp(this, &VAMaterialInspectorPlugin::on_material_selected).bind(node, option_button));

    container->add_child(option_button);
    add_custom_control(container);
}

void VAMaterialInspectorPlugin::on_material_selected(int32_t index, Node *node, OptionButton *option_button)
{
    StringName material_meta_key = VAWorld::get_material_meta_key();

    if (index == 0)
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, option_button->get_item_text(index));

    EditorInterface::get_singleton()->mark_scene_as_unsaved();
}
