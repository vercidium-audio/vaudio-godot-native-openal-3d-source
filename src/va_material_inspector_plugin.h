#pragma once

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace va_godot
{

// Adds a "Vercidium Audio" group with a material dropdown to the Inspector for every Node3D, so
// geometry can be assigned a vaudio material without hand-typing the "vercidium_audio_material"
// metadata string. Shown on all Node3D types (not just MeshInstance3D/CSGShape3D) because
// VAWorld::add_primitive applies a node's material to every descendant that doesn't override it -
// setting it on a plain Node3D group is a normal way to material an entire subtree at once. The
// dropdown lists the 23 built-in materials plus any VACustomMaterial nodes found in the edited scene.
class VAMaterialInspectorPlugin : public EditorInspectorPlugin
{
    GDCLASS(VAMaterialInspectorPlugin, EditorInspectorPlugin);

private:
    void on_material_selected(int32_t index, Node *node, OptionButton *option_button);

protected:
    static void _bind_methods();

public:
    bool _can_handle(Object *object) const override;
    void _parse_end(Object *object) override;
};

} // namespace va_godot
