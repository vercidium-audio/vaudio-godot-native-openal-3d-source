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

// Adds a "Refresh OpenAL Devices" button right below device_name/
// capture_device_name under VAOpenALSettings/VAInputStreamSource in the
// Inspector - those properties' dropdowns (each class's own
// _validate_property) are only re-queried when Godot rebuilds the node's
// cached property list, which doesn't happen on its own when a device is
// plugged in/unplugged while the node stays selected. The button calls the
// node's own refresh_devices(), which re-queries the driver and forces that
// rebuild via notify_property_list_changed. Handles both classes (rather
// than one plugin per class) since the button is identical either way -
// just wired to whichever node's own refresh_devices() method, and placed
// via _parse_property (called once per property, in list order) instead of
// _parse_end so it lands inline in that node's own section instead of at
// the very bottom of the Inspector, after every other section - see the
// .cpp for why it matches on the NEXT property after the device dropdown(s)
// rather than the dropdown property itself.
class VADeviceRefreshInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VADeviceRefreshInspectorPlugin, EditorInspectorPlugin);

protected:
    static void _bind_methods();

public:
    bool _can_handle(Object *object) const override;
    bool _parse_property(Object *object, Variant::Type type, const String &name, PropertyHint hint_type,
        const String &hint_string, BitField<PropertyUsageFlags> usage_flags, bool wide) override;
};

class VAConversionPlugin : public EditorPlugin
{
    GDCLASS(VAConversionPlugin, EditorPlugin);

private:
    Ref<ConversionContextMenuPlugin> context_menu_plugin;
    Ref<VADeviceRefreshInspectorPlugin> device_refresh_inspector_plugin;
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
