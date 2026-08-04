#include "va_conversion_plugin.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "al_source_node.h"
#include "va_source.h"
#include "va_source_relative.h"

using namespace va_godot;

namespace
{

// AudioStreamPlayer3D is accessed generically (no godot-cpp header is
// generated for it, since nothing else in this plugin needs one), so
// property existence has to be checked at runtime via get_property_list -
// without this, a typo'd or engine-version-specific property name would
// silently no-op via Object::set rather than fail to compile.
bool has_property(Object *object, const StringName &name)
{
    TypedArray<Dictionary> properties = object->get_property_list();

    for (int i = 0; i < properties.size(); i++)
    {
        Dictionary info = properties[i];
        if (String(info["name"]) == String(name))
        {
            return true;
        }
    }

    return false;
}

// Sets `to` on `target` from `from` on `source`, if `source` actually has
// `from` - see has_property above.
bool copy_property(Object *source, Object *target, const StringName &from, const StringName &to)
{
    if (!has_property(source, from))
    {
        return false;
    }

    target->set(to, source->get(from));
    return true;
}

} // namespace

void ConversionContextMenuPlugin::_bind_methods()
{
}

// Convert-to items only show up for a single selected AudioStreamPlayer3D
// (or an existing VASource/VASourceRelative, so users can switch between the
// two without going back to AudioStreamPlayer3D first) - multi-selection
// conversion isn't supported since every node needs its own reparent/owner
// fixup and there's no undo grouping across them yet.
void ConversionContextMenuPlugin::_popup_menu(const PackedStringArray &paths)
{
    if (paths.size() != 1)
    {
        return;
    }

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
    {
        return;
    }

    Node *node = scene_root->get_node_or_null(NodePath(paths[0]));
    if (!node)
    {
        return;
    }

    bool is_audio_player_3d = node->get_class() == "AudioStreamPlayer3D";
    bool is_va_source = Object::cast_to<VASource>(node) != nullptr;
    bool is_va_source_relative = Object::cast_to<VASourceRelative>(node) != nullptr;

    if (!is_audio_player_3d && !is_va_source && !is_va_source_relative)
    {
        return;
    }

    if (!is_va_source)
    {
        add_context_menu_item("Convert to VASource", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASource"));
    }

    if (!is_va_source_relative)
    {
        add_context_menu_item("Convert to VASourceRelative", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASourceRelative"));
    }
}

void ConversionContextMenuPlugin::convert_selected_to(const TypedArray<Node> &nodes, const String &target_class)
{
    if (nodes.size() != 1)
    {
        return;
    }

    convert_node(Object::cast_to<Node>(nodes[0]), target_class);
}

// Creates a new `target_class` node in `old_node`'s place, copies across the
// transform/name/known-equivalent properties, moves `old_node`'s children
// onto it, and frees `old_node`. AudioStreamPlayer3D -> VASource/
// VASourceRelative maps volume_db (dB) -> gain (linear), pitch_scale ->
// pitch, and autoplay/stream/max_distance/unit_size directly by name where
// VASource/VASourceRelative have a matching property; VASource <->
// VASourceRelative just carries over the ALSourceNode-level fields they both
// share (stream/gain/pitch/looping/autoplay), since a relative source has no
// 3D-only max_distance/reference_distance to preserve.
void ConversionContextMenuPlugin::convert_node(Node *old_node, const String &target_class)
{
    if (!old_node)
    {
        return;
    }

    Node *parent = old_node->get_parent();
    if (!parent)
    {
        UtilityFunctions::push_warning("[vaudio-godot-native-openal] Cannot convert the scene root node.");
        return;
    }

    ALSourceNode *new_node = nullptr;

    if (target_class == "VASource")
    {
        new_node = memnew(VASource);
    }
    else if (target_class == "VASourceRelative")
    {
        new_node = memnew(VASourceRelative);
    }
    else
    {
        return;
    }

    Node *new_base_node = Object::cast_to<Node>(new_node);
    String old_name = old_node->get_name();
    int old_index = old_node->get_index();
    Node *owner = old_node->get_owner();

    // AudioStreamPlayer3D's volume_db is logarithmic; VASource/
    // VASourceRelative's gain is linear (matches ALSource's alSourcef(AL_GAIN)
    // convention) - convert rather than a straight name copy.
    if (has_property(old_node, "volume_db"))
    {
        double volume_db = old_node->get("volume_db");
        new_base_node->set("gain", UtilityFunctions::db_to_linear(volume_db));
    }

    copy_property(old_node, new_base_node, "stream", "stream");
    copy_property(old_node, new_base_node, "pitch_scale", "pitch");
    copy_property(old_node, new_base_node, "autoplay", "autoplay");

    // Only meaningful on the 3D/positional VASource - VASourceRelative's
    // sources are pinned to the listener and have no distance attenuation.
    if (target_class == "VASource")
    {
        copy_property(old_node, new_base_node, "max_distance", "max_distance");
        copy_property(old_node, new_base_node, "unit_size", "reference_distance");
    }

    // Converting between VASource and VASourceRelative directly (not from
    // AudioStreamPlayer3D) - carry over the shared ALSourceNode fields that
    // a straight name copy handles fine (both sides already use these exact
    // names/types).
    if (target_class != old_node->get_class())
    {
        copy_property(old_node, new_base_node, "gain", "gain");
        copy_property(old_node, new_base_node, "pitch", "pitch");
        copy_property(old_node, new_base_node, "looping", "looping");
        copy_property(old_node, new_base_node, "autoplay", "autoplay");

        if (target_class == "VASource")
        {
            copy_property(old_node, new_base_node, "max_distance", "max_distance");
            copy_property(old_node, new_base_node, "reference_distance", "reference_distance");
        }
    }

    Node3D *old_node_3d = Object::cast_to<Node3D>(old_node);
    Node3D *new_node_3d = Object::cast_to<Node3D>(new_base_node);
    if (old_node_3d && new_node_3d)
    {
        new_node_3d->set_transform(old_node_3d->get_transform());
    }

    // Move every child across before old_node is freed - add_child below
    // reparents them (Godot detaches a node from its previous parent
    // automatically), so this must happen before old_node leaves the tree.
    while (old_node->get_child_count() > 0)
    {
        Node *child = old_node->get_child(0);
        old_node->remove_child(child);
        new_base_node->add_child(child);
        child->set_owner(owner);
    }

    parent->remove_child(old_node);
    old_node->queue_free();

    new_base_node->set_name(old_name);
    parent->add_child(new_base_node);
    parent->move_child(new_base_node, old_index);
    new_base_node->set_owner(owner);

    EditorInterface::get_singleton()->get_selection()->clear();
    EditorInterface::get_singleton()->get_selection()->add_node(new_base_node);
    EditorInterface::get_singleton()->mark_scene_as_unsaved();
}

void VAConversionPlugin::_bind_methods()
{
}

void VAConversionPlugin::_enter_tree()
{
    context_menu_plugin.instantiate();
    add_context_menu_plugin(EditorContextMenuPlugin::CONTEXT_SLOT_SCENE_TREE, context_menu_plugin);
}

void VAConversionPlugin::_exit_tree()
{
    remove_context_menu_plugin(context_menu_plugin);
    context_menu_plugin.unref();
}
