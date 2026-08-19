#pragma once

#include <godot_cpp/classes/editor_context_menu_plugin.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "va_debugger_plugin.h"
#include "va_material_inspector_plugin.h"
#include "va_world_gizmo.h"

using namespace godot;

namespace va_godot
{

// Right-click to convert AudioStreamPlayer3D to VASource / VASourceRelative / VASourceLeech / VASourceAmbient, or AudioStreamPlayer to VASourceRelative / VASourceAmbient
class ConversionContextMenuPlugin : public EditorContextMenuPlugin
{
    GDCLASS(ConversionContextMenuPlugin, EditorContextMenuPlugin);

private:
    void convert_selected_to(const TypedArray<Node> &nodes, const String &target_class);
    void convert_node(Node *old_node, const String &target_class);

protected:
    static void _bind_methods();

public:
    void _popup_menu(const PackedStringArray &paths) override;
};

// Adds a "Refresh Devices" button under VAOpenALSettings in the Inspector -
// device_name's dropdown (VAOpenALSettings::_validate_property) is only
// re-queried when Godot rebuilds the node's cached property list, which
// doesn't happen on its own when a playback device is plugged in/unplugged
// while the node stays selected. The button calls VAOpenALSettings::refresh_devices,
// which re-queries the driver and forces that rebuild via notify_property_list_changed.
class VAOpenALSettingsInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAOpenALSettingsInspectorPlugin, EditorInspectorPlugin);

protected:
    static void _bind_methods();

public:
    bool _can_handle(Object *object) const override;
    void _parse_end(Object *object) override;
};

class VAConversionPlugin : public EditorPlugin
{
    GDCLASS(VAConversionPlugin, EditorPlugin);

private:
    Ref<ConversionContextMenuPlugin> context_menu_plugin;
    Ref<VAOpenALSettingsInspectorPlugin> openal_settings_inspector_plugin;
    Ref<VAMaterialInspectorPlugin> material_inspector_plugin;
    Ref<VAWorldGizmoPlugin> world_gizmo_plugin;
    Ref<VADebuggerPlugin> debugger_plugin;

protected:
    static void _bind_methods();

public:
    void _enter_tree() override;
    void _exit_tree() override;
};

} // namespace va_godot
