#include "va_material_inspector_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "al_source.h"
#include "va_custom_material.h"
#include "va_emitter.h"
#include "va_world.h"

using namespace va_godot;

namespace
{

const int FIRST_SELECTABLE_BUILTIN_INDEX = 1;

// Index 0 is the default ("All") and is never written as metadata - it's the absence of the key.
const char *const PROPAGATE_MODES[] = {"All", "Colliders only", "Meshes only"};
const char *const PROPAGATE_MODE_META_VALUES[] = {"", "colliders", "visuals"};
const int PROPAGATE_MODE_COUNT = 3;

const int PROPAGATE_LAYER_COUNT = 20;
const Color PROPAGATE_LAYER_SELECTED_COLOR = Color(0.16f, 0.47f, 0.9f);

Ref<StyleBoxFlat> make_selected_layer_stylebox()
{
    Ref<StyleBoxFlat> sb;
    sb.instantiate();
    sb->set_bg_color(PROPAGATE_LAYER_SELECTED_COLOR);
    sb->set_corner_radius_all((int)Math::round(3 * EditorInterface::get_singleton()->get_editor_scale()));
    return sb;
}

int mask_from_buttons(const TypedArray<Button> &layer_buttons)
{
    int mask = 0;

    for (int i = 0; i < layer_buttons.size(); i++)
    {
        Button *button = Object::cast_to<Button>(layer_buttons[i]);

        if (button && button->is_pressed())
            mask |= (1 << i);
    }

    return mask;
}

// Custom materials only register their name with the running ::VAWorld at runtime, so at edit time this walks the scene tree directly instead.
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
    // VAWorld, VAEmitter/VAListener, and ALSource/VASource* are Node3D too, but they're audio control nodes, not geometry.
    if (Object::cast_to<VAWorld>(object) || Object::cast_to<VAEmitter>(object) || Object::cast_to<ALSource>(object))
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
    // Index 0 ("None") removes the metadata; index 1 ("Air") explicitly stores the "air" material string.
    option_button->add_item("None");
    option_button->add_item("Air");

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

    StringName use_flat_transmission_meta_key = VAWorld::get_use_flat_transmission_meta_key();

    HBoxContainer *permeation_row = memnew(HBoxContainer);
    section->add_child(permeation_row);

    Label *permeation_label = memnew(Label);
    permeation_label->set_text("Use Flat Transmission");
    permeation_label->set_custom_minimum_size(Vector2(140, 0));
    permeation_row->add_child(permeation_label);

    CheckBox *permeation_checkbox = memnew(CheckBox);
    bool use_flat_transmission = node->has_meta(use_flat_transmission_meta_key) ? (bool)node->get_meta(use_flat_transmission_meta_key) : false;
    permeation_checkbox->set_pressed(use_flat_transmission);
    permeation_checkbox->connect("toggled", callable_mp(this, &VAMaterialInspectorPlugin::on_use_flat_transmission_toggled).bind(node));
    permeation_row->add_child(permeation_checkbox);

    // Propagation controls only matter when this node has children to cascade into.
    if (node->get_child_count() > 0)
    {
        StringName propagate_meta_key = VAWorld::get_propagate_meta_key();

        HBoxContainer *propagate_row = memnew(HBoxContainer);
        section->add_child(propagate_row);

        Label *propagate_label = memnew(Label);
        propagate_label->set_text("Propagate To");
        propagate_label->set_tooltip_text("Which child nodes a material set on this node cascades down to.\n\nAll: every child (default)\nColliders only: only collision shape children - skips the visual mesh of a mesh + collider pair\nMeshes only: only mesh / non-collision children");
        propagate_label->set_custom_minimum_size(Vector2(140, 0));
        propagate_row->add_child(propagate_label);

        OptionButton *propagate_button = memnew(OptionButton);
        propagate_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);

        for (int i = 0; i < PROPAGATE_MODE_COUNT; i++)
            propagate_button->add_item(PROPAGATE_MODES[i]);

        int propagate_selected = 0;
        if (node->has_meta(propagate_meta_key))
        {
            String current = String(node->get_meta(propagate_meta_key)).to_lower();

            for (int i = 1; i < PROPAGATE_MODE_COUNT; i++)
            {
                if (current == PROPAGATE_MODE_META_VALUES[i])
                {
                    propagate_selected = i;
                    break;
                }
            }
        }

        propagate_button->select(propagate_selected);
        propagate_button->connect("item_selected", callable_mp(this, &VAMaterialInspectorPlugin::on_propagate_selected).bind(node));
        propagate_row->add_child(propagate_button);

        StringName propagate_layer_meta_key = VAWorld::get_propagate_layer_meta_key();

        HBoxContainer *propagate_layer_row = memnew(HBoxContainer);
        section->add_child(propagate_layer_row);

        Label *propagate_layer_label = memnew(Label);
        propagate_layer_label->set_text("Propagate Layers");
        propagate_layer_label->set_tooltip_text("Optional. When one or more layers are selected, the material only cascades to children that are on one of them.\n\nFor mesh children this is the visual render layer; for collider children it's the parent body's collision layer.\n\nSelect nothing to cascade regardless of layer.");
        propagate_layer_label->set_custom_minimum_size(Vector2(140, 0));
        propagate_layer_row->add_child(propagate_layer_label);

        // A compact grid of 20 toggle buttons, mirroring Godot's built-in layer editor.
        GridContainer *grid = memnew(GridContainer);
        grid->set_columns(10);
        grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);

        int current_mask = node->has_meta(propagate_layer_meta_key) ? (int)node->get_meta(propagate_layer_meta_key) : 0;

        float editor_scale = EditorInterface::get_singleton()->get_editor_scale();

        TypedArray<Button> layer_buttons;

        for (int i = 0; i < PROPAGATE_LAYER_COUNT; i++)
        {
            Button *bit_button = memnew(Button);
            bit_button->set_toggle_mode(true);
            bit_button->set_text(String::num_int64(i + 1));
            bit_button->set_custom_minimum_size(Vector2(26, 26) * editor_scale);
            bit_button->add_theme_font_size_override("font_size", (int)Math::round(10 * editor_scale));
            bit_button->add_theme_color_override("font_pressed_color", Color(1, 1, 1));
            bit_button->add_theme_color_override("font_hover_pressed_color", Color(1, 1, 1));
            bit_button->add_theme_stylebox_override("pressed", make_selected_layer_stylebox());
            bit_button->add_theme_stylebox_override("hover_pressed", make_selected_layer_stylebox());
            bit_button->set_tooltip_text("Layer " + String::num_int64(i + 1));
            bit_button->set_pressed((current_mask & (1 << i)) != 0);
            grid->add_child(bit_button);
            layer_buttons.push_back(bit_button);
        }

        for (int i = 0; i < layer_buttons.size(); i++)
        {
            Button *bit_button = Object::cast_to<Button>(layer_buttons[i]);
            bit_button->connect("toggled", callable_mp(this, &VAMaterialInspectorPlugin::on_propagate_layer_bit_toggled).bind(node, layer_buttons));
        }

        propagate_layer_row->add_child(grid);
    }

    add_custom_control(section);
}

void VAMaterialInspectorPlugin::on_material_selected(int32_t index, Node *node, OptionButton *option_button)
{
    StringName material_meta_key = VAWorld::get_material_meta_key();

    if (index == 0)
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, option_button->get_item_text(index).to_lower());

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::on_use_flat_transmission_toggled(bool toggled_on, Node *node)
{
    StringName use_flat_transmission_meta_key = VAWorld::get_use_flat_transmission_meta_key();

    // false is the default, so only store metadata for the non-default value, matching the "Air" material entry.
    if (!toggled_on)
        node->remove_meta(use_flat_transmission_meta_key);
    else
        node->set_meta(use_flat_transmission_meta_key, true);

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::on_propagate_selected(int32_t index, Node *node)
{
    StringName propagate_meta_key = VAWorld::get_propagate_meta_key();

    if (index == 0)
        node->remove_meta(propagate_meta_key);
    else
        node->set_meta(propagate_meta_key, String(PROPAGATE_MODE_META_VALUES[index]));

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::on_propagate_layer_bit_toggled(bool pressed, Node *node, TypedArray<Button> layer_buttons)
{
    StringName propagate_layer_meta_key = VAWorld::get_propagate_layer_meta_key();

    int mask = mask_from_buttons(layer_buttons);

    // No layers selected means "don't restrict by layer" - store nothing.
    if (mask == 0)
        node->remove_meta(propagate_layer_meta_key);
    else
        node->set_meta(propagate_layer_meta_key, mask);

    EditorInterface::get_singleton()->mark_scene_as_unsaved();

    sync_running_game(node);
}

void VAMaterialInspectorPlugin::sync_running_game(Node *node)
{
    // No way to reach a Node* in the running game's separate process directly, so this sends the node's path (relative to
    // the edited scene root, which the running game's scene root mirrors) plus the current metadata over the debugger protocol.
    if (!debugger_plugin.is_valid())
        return;

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
        return;

    StringName material_meta_key = VAWorld::get_material_meta_key();
    String material = node->has_meta(material_meta_key) ? String(node->get_meta(material_meta_key)) : String();

    StringName use_flat_transmission_meta_key = VAWorld::get_use_flat_transmission_meta_key();
    Variant use_flat_transmission = node->has_meta(use_flat_transmission_meta_key) ? node->get_meta(use_flat_transmission_meta_key) : Variant();

    StringName propagate_meta_key = VAWorld::get_propagate_meta_key();
    String propagate = node->has_meta(propagate_meta_key) ? String(node->get_meta(propagate_meta_key)) : String();

    StringName propagate_layer_meta_key = VAWorld::get_propagate_layer_meta_key();
    Variant propagate_layer = node->has_meta(propagate_layer_meta_key) ? node->get_meta(propagate_layer_meta_key) : Variant();

    NodePath node_path = scene_root->get_path_to(node);
    debugger_plugin->sync_primitive(scene_root->get_name(), node_path, material, use_flat_transmission, propagate, propagate_layer);
}
