#pragma once

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "va_debugger_plugin.h"

using namespace godot;

namespace va_godot
{

// Adds a "Vercidium Audio" group with a material dropdown and a "Use Flat Transmission" checkbox to the Inspector for every
// Node3D, so geometry can be configured without hand-typing the metadata. Shown on all Node3D (not just mesh/CSG types)
// because VAWorld::add_primitive applies a node's material/transmission override to every descendant that doesn't override
// it itself - setting either on a plain Node3D group configures an entire subtree at once.
class VAMaterialInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAMaterialInspectorPlugin, EditorInspectorPlugin);

private:
    // Relays edits to a running game's VAWorld. Null until this plugin's owning VAConversionPlugin finishes _enter_tree.
    Ref<VADebuggerPlugin> debugger_plugin;

    void on_material_selected(int32_t index, Node *node, OptionButton *option_button);
    void on_use_flat_transmission_toggled(bool toggled_on, Node *node);

    void sync_running_game(Node *node);

protected:
    static void _bind_methods();

public:
    bool _can_handle(Object *object) const override;
    void _parse_end(Object *object) override;

    void set_debugger_plugin(const Ref<VADebuggerPlugin> &plugin)
    {
        debugger_plugin = plugin;
    }
};

} // namespace va_godot
