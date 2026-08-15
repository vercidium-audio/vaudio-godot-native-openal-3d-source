#include "va_material_inspector_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/vector2.hpp>

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

    VBoxContainer *section = memnew(VBoxContainer);

    Label *heading = memnew(Label);
    heading->set_text("Vercidium Audio");
    section->add_child(heading);

    HBoxContainer *row = memnew(HBoxContainer);
    section->add_child(row);

    Label *label = memnew(Label);
    label->set_text("Material");
    label->set_custom_minimum_size(Vector2(140, 0));
    row->add_child(label);

    OptionButton *option_button = memnew(OptionButton);
    option_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
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

    row->add_child(option_button);

    StringName supports_permeation_meta_key = VAWorld::get_supports_permeation_meta_key();

    HBoxContainer *permeation_row = memnew(HBoxContainer);
    section->add_child(permeation_row);

    Label *permeation_label = memnew(Label);
    permeation_label->set_text("Supports Permeation");
    permeation_label->set_custom_minimum_size(Vector2(140, 0));
    permeation_row->add_child(permeation_label);

    CheckBox *permeation_checkbox = memnew(CheckBox);
    bool supports_permeation = node->has_meta(supports_permeation_meta_key) ? (bool)node->get_meta(supports_permeation_meta_key) : true;
    permeation_checkbox->set_pressed(supports_permeation);
    permeation_checkbox->connect("toggled", callable_mp(this, &VAMaterialInspectorPlugin::on_supports_permeation_toggled).bind(node));
    permeation_row->add_child(permeation_checkbox);

    add_custom_control(section);
}

void VAMaterialInspectorPlugin::on_material_selected(int32_t index, Node *node, OptionButton *option_button)
{
    StringName material_meta_key = VAWorld::get_material_meta_key();

    if (index == 0)
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, option_button->get_item_text(index));

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::on_supports_permeation_toggled(bool toggled_on, Node *node)
{
    StringName supports_permeation_meta_key = VAWorld::get_supports_permeation_meta_key();

    // true is the default (see add_primitive), so only store metadata for the non-default value -
    // matches the "Air (no geometry)" material entry removing its meta key instead of storing it.
    if (toggled_on)
        node->remove_meta(supports_permeation_meta_key);
    else
        node->set_meta(supports_permeation_meta_key, false);

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::sync_running_game(Node *node)
{
    // Custom EditorInspectorPlugin controls only ever run against the editor's local copy of the
    // scene - there's no way to reach a Node* in the running game's separate process directly, so
    // this instead sends the node's path (relative to the edited scene root, which the running
    // game's scene root mirrors) over the debugger protocol - see VADebuggerPlugin. The running
    // game's copy of this node was never touched by the set_meta/remove_meta call that just ran
    // above on the editor's local copy, so the current metadata values have to be sent too, for the
    // receiving end to apply to its own copy before re-adding the primitive.
    if (!debugger_plugin.is_valid())
        return;

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
        return;

    StringName material_meta_key = VAWorld::get_material_meta_key();
    String material = node->has_meta(material_meta_key) ? String(node->get_meta(material_meta_key)) : String();

    StringName supports_permeation_meta_key = VAWorld::get_supports_permeation_meta_key();
    Variant supports_permeation = node->has_meta(supports_permeation_meta_key) ? node->get_meta(supports_permeation_meta_key) : Variant();

    NodePath node_path = scene_root->get_path_to(node);
    debugger_plugin->sync_primitive(scene_root->get_name(), node_path, material, supports_permeation);
}
