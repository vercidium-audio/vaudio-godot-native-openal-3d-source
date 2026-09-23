#include "register_types.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "openal/al_manager.h"
#include "va_custom_material.h"
#include "va_default_material.h"
#include "va_device_name.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

using namespace godot;

Node *find_child_named_recursive(Node *node, const StringName &name)
{
    TypedArray<Node> children = node->get_children();

    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);

        if (child->get_name() == name)
            return child;

        if (Node *found = find_child_named_recursive(child, name))
            return found;
    }

    return nullptr;
}

static Node *replace_with_material_node(Node *scene_root, const NodePath &node_path, Node *existing_node, const String &node_name,
    bool is_custom_material, int material_type, const String &custom_material_name)
{
    Node *parent;

    if (existing_node)
    {
        parent = existing_node->get_parent();

        if (parent)
            parent->remove_child(existing_node);

        existing_node->queue_free();
    }
    else
    {
        int64_t name_count = node_path.get_name_count();
        if (name_count < 2)
            return nullptr;

        parent = scene_root->get_node_or_null(node_path.slice(0, name_count - 1));
    }

    if (!Object::cast_to<va_godot::VAWorld>(parent))
        return nullptr;

    Node *node;

    if (is_custom_material)
    {
        va_godot::VACustomMaterial *custom_material = memnew(va_godot::VACustomMaterial);
        custom_material->set_material_name(custom_material_name);
        node = custom_material;
    }
    else
    {
        VADefaultMaterial *default_material = memnew(VADefaultMaterial);
        default_material->set_material_type(material_type);
        node = default_material;
    }

    node->set_name(node_name);
    parent->add_child(node);

    return node;
}

static bool on_sync_material_properties(const Array &data)
{
    // scene_root_name, node_path, node_name, is_custom_material, material_type, custom_material_name,
    // 7 material floats, color
    if (data.size() < 14)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material property edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material property edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    String node_name = data[2];
    bool is_custom_material = data[3];
    int material_type = data[4];
    String custom_material_name = data[5];

    float absorption_lf = data[6];
    float absorption_hf = data[7];
    float scattering = data[8];
    float transmission_lf = data[9];
    float transmission_hf = data[10];
    float flat_transmission_lf = data[11];
    float flat_transmission_hf = data[12];
    Color color = data[13];

    if (!Object::cast_to<VADefaultMaterial>(node) && !Object::cast_to<va_godot::VACustomMaterial>(node))
        node = replace_with_material_node(scene_root, node_path, node, node_name, is_custom_material, material_type, custom_material_name);

    if (VADefaultMaterial *default_material = Object::cast_to<VADefaultMaterial>(node))
        default_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else if (va_godot::VACustomMaterial *custom_material = Object::cast_to<va_godot::VACustomMaterial>(node))
        custom_material->apply_properties_from_editor(absorption_lf, absorption_hf, scattering,
            transmission_lf, transmission_hf, flat_transmission_lf, flat_transmission_hf, color);
    else
        VA_WARN(
            "Received a material property edit from the editor for '", node_path,
            "', but no matching VADefaultMaterial/VACustomMaterial node exists under '", scene_root_name, "' (", scene_root->get_path(),
            "), and its parent VAWorld node doesn't exist in the running scene either - restart the running game to pick it up");

    return true;
}

bool on_debugger_message(const String &message, const Array &data)
{
    if (message == "sync_material_properties")
        return on_sync_material_properties(data);

    if (message == "sync_viewport_camera")
        return on_sync_viewport_camera(data);

    if (message != "sync_primitive" || data.size() < 4)
        return false;

    SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *tree_root = scene_tree ? scene_tree->get_root() : nullptr;

    if (!tree_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but the game has no scene tree root");
        return true;
    }

    StringName scene_root_name = data[0];
    Node *scene_root = find_child_named_recursive(tree_root, scene_root_name);

    if (!scene_root)
    {
        VA_WARN("Received a material/permeation edit from the editor, but no node named '", scene_root_name, "' exists in the running scene");
        return true;
    }

    NodePath node_path = data[1];
    Node *node = scene_root->get_node_or_null(node_path);

    if (!node)
    {
        VA_WARN(
            "Received a material/permeation edit from the editor for '", node_path,
            "', but no matching node exists under '", scene_root_name, "' (", scene_root->get_path(), ")");
        return true;
    }

    va_godot::VAWorld *va_world = va_godot::find_va_world(node);

    if (!va_world)
    {
        VA_WARN("Received a material/permeation edit from the editor for '", node->get_name(), "', but couldn't find a VAWorld in the running scene");
        return true;
    }

    // The running game has its own separate copy of this node - apply the metadata the editor's local copy just had
    // set/removed on it before re-adding the primitive below, since add_primitive/get_material read it straight off this node.
    String material = data[2];
    StringName material_meta_key = va_godot::VAWorld::get_material_meta_key();

    if (material.is_empty())
        node->remove_meta(material_meta_key);
    else
        node->set_meta(material_meta_key, material);

    Variant use_flat_transmission = data[3];
    StringName use_flat_transmission_meta_key = va_godot::VAWorld::get_use_flat_transmission_meta_key();

    if (use_flat_transmission.get_type() == Variant::NIL)
        node->remove_meta(use_flat_transmission_meta_key);
    else
        node->set_meta(use_flat_transmission_meta_key, use_flat_transmission);

    // Propagation filter (optional - older editor builds send a 4-element payload).
    if (data.size() > 4)
    {
        String propagate = data[4];
        StringName propagate_meta_key = va_godot::VAWorld::get_propagate_meta_key();

        if (propagate.is_empty())
            node->remove_meta(propagate_meta_key);
        else
            node->set_meta(propagate_meta_key, propagate);
    }

    va_world->sync_primitive(node);

    return true;
}

void refresh_output_device_hint()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    PackedStringArray devices = ALManager::get_singleton() ? ALManager::get_singleton()->get_available_devices() : PackedStringArray();
    devices.insert(0, DEFAULT_DEVICE_LABEL);

    Dictionary output_device_info;
    output_device_info["name"] = "audio/vaudio/output_device";
    output_device_info["type"] = Variant::STRING;
    output_device_info["hint"] = PROPERTY_HINT_ENUM;
    output_device_info["hint_string"] = String(",").join(devices);
    settings->add_property_info(output_device_info);
}

// Registers this plugin's own audio/vaudio/* entries under Project Settings - device_name/max_reverb_sends/sample_rate/
// hrtf_enabled must exist here so ALManager::initialize() can read them before it opens the one-and-only device, below.
void register_project_settings()
{
    ProjectSettings *settings = ProjectSettings::get_singleton();

    // output_device: stored as DEFAULT_DEVICE_LABEL, not "", so the strict PROPERTY_HINT_ENUM dropdown below always has
    // a current value among its own entries. ALManager::read_settings_from_project_settings() translates it back to "" ("driver default").
    if (!settings->has_setting("audio/vaudio/output_device"))
        settings->set_setting("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    settings->set_initial_value("audio/vaudio/output_device", DEFAULT_DEVICE_LABEL);

    refresh_output_device_hint();

    if (!settings->has_setting("audio/vaudio/max_reverb_sends"))
        settings->set_setting("audio/vaudio/max_reverb_sends", 1);

    settings->set_initial_value("audio/vaudio/max_reverb_sends", 1);

    Dictionary max_reverb_sends_info;
    max_reverb_sends_info["name"] = "audio/vaudio/max_reverb_sends";
    max_reverb_sends_info["type"] = Variant::INT;
    max_reverb_sends_info["hint"] = PROPERTY_HINT_RANGE;
    max_reverb_sends_info["hint_string"] = "1,16,or_greater";
    settings->add_property_info(max_reverb_sends_info);

    // max_mono_sources/max_stereo_sources: project-level settings set by the developer, matching ALManager.cs's
    // MaximumMonoSources/MaximumStereoSources defaults (16/240). Read once before the one-and-only device open, not settable at runtime.
    if (!settings->has_setting("audio/vaudio/max_mono_sources"))
        settings->set_setting("audio/vaudio/max_mono_sources", 16);

    settings->set_initial_value("audio/vaudio/max_mono_sources", 16);

    Dictionary max_mono_sources_info;
    max_mono_sources_info["name"] = "audio/vaudio/max_mono_sources";
    max_mono_sources_info["type"] = Variant::INT;
    max_mono_sources_info["hint"] = PROPERTY_HINT_RANGE;
    max_mono_sources_info["hint_string"] = "0,256,or_greater";
    settings->add_property_info(max_mono_sources_info);

    if (!settings->has_setting("audio/vaudio/max_stereo_sources"))
        settings->set_setting("audio/vaudio/max_stereo_sources", 240);

    settings->set_initial_value("audio/vaudio/max_stereo_sources", 240);

    Dictionary max_stereo_sources_info;
    max_stereo_sources_info["name"] = "audio/vaudio/max_stereo_sources";
    max_stereo_sources_info["type"] = Variant::INT;
    max_stereo_sources_info["hint"] = PROPERTY_HINT_RANGE;
    max_stereo_sources_info["hint_string"] = "0,256,or_greater";
    settings->add_property_info(max_stereo_sources_info);

    // sample_rate: 0 means "driver default" - never shown to the user as 0.
    if (!settings->has_setting("audio/vaudio/sample_rate"))
        settings->set_setting("audio/vaudio/sample_rate", 0);

    settings->set_initial_value("audio/vaudio/sample_rate", 0);

    Dictionary sample_rate_info;
    sample_rate_info["name"] = "audio/vaudio/sample_rate";
    sample_rate_info["type"] = Variant::INT;
    sample_rate_info["hint"] = PROPERTY_HINT_ENUM;
    sample_rate_info["hint_string"] = "System Default:0,22050,44100,48000,96000";
    settings->add_property_info(sample_rate_info);

    // hrtf_enabled: default true
    if (!settings->has_setting("audio/vaudio/hrtf_enabled"))
        settings->set_setting("audio/vaudio/hrtf_enabled", true);

    settings->set_initial_value("audio/vaudio/hrtf_enabled", true);
}
