#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "va_debugger_plugin.h"

using namespace godot;

namespace va_godot
{

class VAMaterialInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAMaterialInspectorPlugin, EditorInspectorPlugin);

private:
    // Relays edits to a running game's VAWorld. Null until this plugin's owning VAConversionPlugin finishes _enter_tree.
    Ref<VADebuggerPlugin> debugger_plugin;

    void on_material_selected(int32_t index, Node *node, OptionButton *option_button);
    void on_use_flat_transmission_toggled(bool toggled_on, Node *node);
    void on_propagate_selected(int32_t index, Node *node);
    void on_propagate_layer_bit_toggled(bool pressed, Node *node, TypedArray<Button> layer_buttons);

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
