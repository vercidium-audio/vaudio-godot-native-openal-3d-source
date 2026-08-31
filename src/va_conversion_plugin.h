#pragma once

#include <godot_cpp/classes/editor_context_menu_plugin.hpp>
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "va_debugger_plugin.h"
#include "va_material_inspector_plugin.h"
#include "va_material_properties_inspector_plugin.h"
#include "va_node_gizmo.h"
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

public:
    // VAWorld looks this up via Engine::get_singleton to reach debugger_plugin, since VAWorld is instantiated by the user's own scene, not this plugin.
    static constexpr const char *DEBUGGER_PLUGIN_SINGLETON_NAME = "VADebuggerPlugin";

private:
    Ref<ConversionContextMenuPlugin> context_menu_plugin;
    Ref<VADeviceRefreshInspectorPlugin> device_refresh_inspector_plugin;
    Ref<VAMaterialInspectorPlugin> material_inspector_plugin;
    Ref<VAMaterialPropertiesInspectorPlugin> material_properties_inspector_plugin;
    Ref<VAWorldGizmoPlugin> world_gizmo_plugin;
    Ref<VANodeGizmoPlugin> node_gizmo_plugin;
    Ref<VADebuggerPlugin> debugger_plugin;

    void refresh_output_device_setting();

protected:
    static void _bind_methods();

public:
    void _enter_tree() override;
    void _exit_tree() override;
};

} // namespace va_godot
