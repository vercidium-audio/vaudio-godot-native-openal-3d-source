#pragma once

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "va_debugger_plugin.h"

using namespace godot;

namespace va_godot
{

// VADefaultMaterial/VACustomMaterial's setters (set_absorption_lf, set_scattering, etc.) only push their new value
// into a running ::VAWorld when registered is true, which is only ever set by their own _enter_tree - and that bails
// out immediately when IS_EDITOR_HINT() is true. So editing one of these nodes' properties in the Inspector while the
// game is running has no effect on the actual running raytracing world, exactly like the material-assignment dropdown
// VAMaterialInspectorPlugin fixes for node metadata. Unlike that dropdown, these are plain Godot-drawn float/color
// editors rather than a custom control, so there is no per-control "selected" signal to hook - instead this connects
// once to the shared EditorInspector's property_edited signal (not exposed as a typed godot-cpp binding, so connected
// by string name) and filters by the currently edited object's type.
class VAMaterialPropertiesInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAMaterialPropertiesInspectorPlugin, EditorInspectorPlugin);

private:
    // Relays edits to a running game's VAWorld. Null until this plugin's owning VAConversionPlugin finishes _enter_tree.
    Ref<VADebuggerPlugin> debugger_plugin;

    // The object currently being drawn by this plugin pass - _can_handle runs once per inspected object right before
    // Godot asks this plugin to parse its properties, so it doubles as the "which object is the property_edited
    // signal about" filter without needing EditorInterface to expose that directly. Mutable since _can_handle is
    // const (inherited from EditorInspectorPlugin) but still needs to record this for on_property_edited to read later.
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
