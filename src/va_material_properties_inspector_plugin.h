#pragma once

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "va_debugger_plugin.h"

using namespace godot;

namespace va_godot
{

class VAMaterialPropertiesInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAMaterialPropertiesInspectorPlugin, EditorInspectorPlugin);

private:
    // Relays edits to a running game's VAWorld. Null until this plugin's owning VAConversionPlugin finishes _enter_tree.
    Ref<VADebuggerPlugin> debugger_plugin;

    mutable Node *current_node = nullptr;

    void on_property_edited(const String &property_name);
    void sync_running_game(Node *node);

protected:
    static void _bind_methods();

public:
    bool _can_handle(Object *object) const override;

    void set_debugger_plugin(const Ref<VADebuggerPlugin> &plugin);
};

} // namespace va_godot
