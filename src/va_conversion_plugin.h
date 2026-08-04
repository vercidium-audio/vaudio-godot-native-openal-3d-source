#pragma once

#include <godot_cpp/classes/editor_context_menu_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

namespace va_godot
{

// Right-click "Convert to VASource" / "Convert to VASourceRelative" scene
// tree context menu items for AudioStreamPlayer3D nodes (and for converting
// directly between VASource/VASourceRelative). Godot's built-in "Change
// Type" dialog only copies properties with matching names/types, which
// doesn't help for e.g. AudioStreamPlayer3D's `pitch_scale` -> VASource's
// `pitch`, so this ports across the known-equivalent properties by hand
// instead. See ConversionContextMenuPlugin::convert_node for the mapping.
class ConversionContextMenuPlugin : public EditorContextMenuPlugin
{
    GDCLASS(ConversionContextMenuPlugin, EditorContextMenuPlugin);

private:
    // Bound as the context menu item's callback via callable_mp(...).bind(
    // target_class) - Godot calls a scene-tree context menu item's callback
    // with the selected Node objects themselves (not the paths given to
    // _popup_menu), so this must accept them even though only nodes[0] is
    // used (multi-selection conversion isn't supported - see _popup_menu).
    void convert_selected_to(const TypedArray<Node> &nodes, const String &target_class);
    void convert_node(Node *old_node, const String &target_class);

protected:
    static void _bind_methods();

public:
    void _popup_menu(const PackedStringArray &paths) override;
};

class VAConversionPlugin : public EditorPlugin
{
    GDCLASS(VAConversionPlugin, EditorPlugin);

private:
    Ref<ConversionContextMenuPlugin> context_menu_plugin;

protected:
    static void _bind_methods();

public:
    void _enter_tree() override;
    void _exit_tree() override;
};

} // namespace va_godot
