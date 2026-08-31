#include "va_conversion_plugin.h"

#include <godot_cpp/classes/audio_stream_ogg_vorbis.hpp>
#include <godot_cpp/classes/audio_stream_randomizer.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "al_source.h"
#include "register_types.h"
#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_input_stream_source.h"
#include "va_source.h"
#include "va_source_ambient.h"
#include "va_source_leech.h"
#include "va_source_relative.h"

using namespace va_godot;

namespace
{

// AudioStreamPlayer3D is accessed generically (no godot-cpp header is generated for it), so property existence has to be checked at runtime via get_property_list
bool has_property(Object *object, const StringName &name)
{
    TypedArray<Dictionary> properties = object->get_property_list();

    for (int i = 0; i < properties.size(); i++)
    {
        Dictionary info = properties[i];

        if (String(info["name"]) == String(name))
            return true;
    }

    return false;
}

bool copy_property(Object *source, Object *target, const StringName &from, const StringName &to)
{
    if (!has_property(source, from))
        return false;

    target->set(to, source->get(from));
    return true;
}

bool any_stream_wants_looping(const TypedArray<AudioStream> &streams)
{
    for (int i = 0; i < streams.size(); i++)
    {
        Ref<AudioStream> stream = streams[i];

        AudioStreamWAV *wav = Object::cast_to<AudioStreamWAV>(stream.ptr());
        if (wav && wav->get_loop_mode() != AudioStreamWAV::LOOP_DISABLED)
            return true;

        AudioStreamOggVorbis *ogg = Object::cast_to<AudioStreamOggVorbis>(stream.ptr());
        if (ogg && ogg->has_loop())
            return true;
    }

    return false;
}

} // namespace

void ConversionContextMenuPlugin::_bind_methods()
{
}

// Menu items are only offered when every selected node is eligible for that conversion, since convert_node() runs once per node with no shared undo grouping across them.
void ConversionContextMenuPlugin::_popup_menu(const PackedStringArray &paths)
{
    if (paths.is_empty())
        return;

    Node *scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root)
        return;

    bool any_eligible = false;
    bool all_not_va_source = true;
    bool all_not_va_source_relative = true;
    bool all_not_va_source_leech_and_parent_is_emitter = true;
    bool all_not_va_source_ambient = true;

    for (int i = 0; i < paths.size(); i++)
    {
        Node *node = scene_root->get_node_or_null(NodePath(paths[i]));
        if (!node)
            return;

        bool is_audio_player_3d = node->get_class() == "AudioStreamPlayer3D";
        bool is_audio_player = node->get_class() == "AudioStreamPlayer";
        bool is_va_source = Object::cast_to<VASource>(node) != nullptr;
        bool is_va_source_relative = Object::cast_to<VASourceRelative>(node) != nullptr;
        bool is_va_source_leech = Object::cast_to<VASourceLeech>(node) != nullptr;
        bool is_va_source_ambient = Object::cast_to<VASourceAmbient>(node) != nullptr;

        if (!is_audio_player_3d && !is_audio_player && !is_va_source && !is_va_source_relative && !is_va_source_leech && !is_va_source_ambient)
            return;

        any_eligible = true;

        // AudioStreamPlayer isn't spatialised, so it can only become a VASourceRelative/VASourceAmbient - never offer VASource/VASourceLeech for it.
        if (is_audio_player)
            all_not_va_source = all_not_va_source_leech_and_parent_is_emitter = false;

        if (is_va_source)
            all_not_va_source = false;

        if (is_va_source_relative)
            all_not_va_source_relative = false;

        if (is_va_source_ambient)
            all_not_va_source_ambient = false;

        // VASourceLeech must be a direct child of a VAEmitter to function (it leeches that parent's raytracing results instead of casting its own).
        bool parent_is_va_emitter = Object::cast_to<VAEmitter>(node->get_parent()) != nullptr;

        if (is_va_source_leech || !parent_is_va_emitter)
            all_not_va_source_leech_and_parent_is_emitter = false;
    }

    if (!any_eligible)
        return;

    if (all_not_va_source)
        add_context_menu_item("Convert to VASource", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASource"));

    if (all_not_va_source_relative)
        add_context_menu_item("Convert to VASourceRelative", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASourceRelative"));

    if (all_not_va_source_leech_and_parent_is_emitter)
        add_context_menu_item("Convert to VASourceLeech", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASourceLeech"));

    if (all_not_va_source_ambient)
        add_context_menu_item("Convert to VASourceAmbient", callable_mp(this, &ConversionContextMenuPlugin::convert_selected_to).bind("VASourceAmbient"));
}

void ConversionContextMenuPlugin::convert_selected_to(const TypedArray<Node> &nodes, const String &target_class)
{
    for (int i = 0; i < nodes.size(); i++)
        convert_node(Object::cast_to<Node>(nodes[i]), target_class);
}

void ConversionContextMenuPlugin::convert_node(Node *old_node, const String &target_class)
{
    if (!old_node)
        return;

    Node *parent = old_node->get_parent();
    if (!parent)
    {
        VA_WARN("Cannot convert the scene root node.");
        return;
    }

    ALSource *new_node = nullptr;

    if (target_class == "VASource")
        new_node = memnew(VASource);
    else if (target_class == "VASourceRelative")
        new_node = memnew(VASourceRelative);
    else if (target_class == "VASourceLeech")
        new_node = memnew(VASourceLeech);
    else if (target_class == "VASourceAmbient")
        new_node = memnew(VASourceAmbient);
    else
        return;

    Node *new_base_node = Object::cast_to<Node>(new_node);
    String old_name = old_node->get_name();
    int old_index = old_node->get_index();
    Node *owner = old_node->get_owner();

    // Convert logarithmin volume_db to linear gain
    if (has_property(old_node, "volume_db"))
    {
        double volume_db = old_node->get("volume_db");
        new_base_node->set("gain", UtilityFunctions::db_to_linear(volume_db));
    }

    copy_property(old_node, new_base_node, "pitch_scale", "pitch");
    copy_property(old_node, new_base_node, "autoplay", "autoplay");

    if (has_property(old_node, "stream"))
    {
        Ref<AudioStream> old_stream = old_node->get("stream");
        AudioStreamRandomizer *randomizer = Object::cast_to<AudioStreamRandomizer>(old_stream.ptr());

        if (randomizer)
        {
            TypedArray<AudioStream> extracted_streams;

            for (int i = 0; i < randomizer->get_streams_count(); i++)
            {
                Ref<AudioStream> sub_stream = randomizer->get_stream(i);

                if (sub_stream.is_valid())
                    extracted_streams.push_back(sub_stream);
            }

            new_base_node->set("streams", extracted_streams);
            new_base_node->set("pitch_randomness", randomizer->get_random_pitch());
            new_base_node->set("volume_randomness_db", randomizer->get_random_volume_offset_db());
        }
        else if (old_stream.is_valid())
        {
            TypedArray<AudioStream> single_stream;
            single_stream.push_back(old_stream);
            new_base_node->set("streams", single_stream);
        }
    }

    bool is_spatialised_target = target_class == "VASource" || target_class == "VASourceLeech";

    if (is_spatialised_target)
    {
        // Spatialised properties
        copy_property(old_node, new_base_node, "max_distance", "max_distance");
        copy_property(old_node, new_base_node, "unit_size", "reference_distance");
    }

    if (target_class != old_node->get_class())
    {
        copy_property(old_node, new_base_node, "gain", "gain");
        copy_property(old_node, new_base_node, "pitch", "pitch");
        copy_property(old_node, new_base_node, "looping", "looping");
        copy_property(old_node, new_base_node, "autoplay", "autoplay");

        // Only relevant when old_node is itself an ALSource - has_property() skips these silently for a plain AudioStreamPlayer3D.
        copy_property(old_node, new_base_node, "streams", "streams");
        copy_property(old_node, new_base_node, "pitch_randomness", "pitch_randomness");
        copy_property(old_node, new_base_node, "volume_randomness_db", "volume_randomness_db");
        copy_property(old_node, new_base_node, "playback_no_repeat", "playback_no_repeat");

        if (is_spatialised_target)
        {
            copy_property(old_node, new_base_node, "max_distance", "max_distance");
            copy_property(old_node, new_base_node, "reference_distance", "reference_distance");
        }
    }

    if (!has_property(old_node, "looping"))
    {
        TypedArray<AudioStream> streams = new_base_node->get("streams");

        new_base_node->set("looping", any_stream_wants_looping(streams));
    }

    // If 3D, copy its transform
    Node3D *old_node_3d = Object::cast_to<Node3D>(old_node);
    Node3D *new_node_3d = Object::cast_to<Node3D>(new_base_node);

    if (old_node_3d && new_node_3d)
        new_node_3d->set_transform(old_node_3d->get_transform());

    // Move children across before the old node is freed
    while (old_node->get_child_count() > 0)
    {
        Node *child = old_node->get_child(0);
        old_node->remove_child(child);
        new_base_node->add_child(child);
        child->set_owner(owner);
    }

    parent->remove_child(old_node);
    old_node->queue_free();

    new_base_node->set_name(old_name);
    parent->add_child(new_base_node);
    parent->move_child(new_base_node, old_index);
    new_base_node->set_owner(owner);

    EditorInterface::get_singleton()->get_selection()->clear();
    EditorInterface::get_singleton()->get_selection()->add_node(new_base_node);
    EditorInterface::get_singleton()->mark_scene_as_unsaved();
}

void VADeviceRefreshInspectorPlugin::_bind_methods()
{
}

bool VADeviceRefreshInspectorPlugin::_can_handle(Object *object) const
{
    return Object::cast_to<VAInputStreamSource>(object) != nullptr;
}

bool VADeviceRefreshInspectorPlugin::_parse_property(Object *object, Variant::Type type, const String &name, PropertyHint hint_type,
    const String &hint_string, BitField<PropertyUsageFlags> usage_flags, bool wide)
{
    // add_custom_control() inserts the control just before the current property's own row is drawn, not after it - so to
    // land the button visually below device_name, this matches on the NEXT property (buffer_size_frames) instead.
    bool is_input_stream_target = Object::cast_to<VAInputStreamSource>(object) && name == StringName("buffer_size_frames");

    if (!is_input_stream_target)
        return false;

    Button *refresh_button = memnew(Button);
    refresh_button->set_text("Refresh OpenAL Devices");
    refresh_button->connect("pressed", Callable(object, "refresh_devices"));
    add_custom_control(refresh_button);

    return false;
}

void VAConversionPlugin::_bind_methods()
{
}

void VAConversionPlugin::refresh_output_device_setting()
{
    refresh_output_device_hint();
}

void VAConversionPlugin::_enter_tree()
{
    context_menu_plugin.instantiate();
    add_context_menu_plugin(EditorContextMenuPlugin::CONTEXT_SLOT_SCENE_TREE, context_menu_plugin);

    device_refresh_inspector_plugin.instantiate();
    add_inspector_plugin(device_refresh_inspector_plugin);

    debugger_plugin.instantiate();
    add_debugger_plugin(debugger_plugin);

    // Expose it to VAWorld via a plain-Object bridge singleton - see DEBUGGER_BRIDGE_SINGLETON_NAME. VADebuggerPlugin itself is RefCounted and can't be registered as a singleton without Godot warning about a dangling raw pointer.
    debugger_bridge = memnew(VADebuggerBridge);
    debugger_bridge->set_debugger_plugin(debugger_plugin.ptr());
    Engine::get_singleton()->register_singleton(DEBUGGER_BRIDGE_SINGLETON_NAME, debugger_bridge);

    material_inspector_plugin.instantiate();
    material_inspector_plugin->set_debugger_plugin(debugger_plugin);
    add_inspector_plugin(material_inspector_plugin);

    material_properties_inspector_plugin.instantiate();
    material_properties_inspector_plugin->set_debugger_plugin(debugger_plugin);
    add_inspector_plugin(material_properties_inspector_plugin);

    world_gizmo_plugin.instantiate();
    add_node_3d_gizmo_plugin(world_gizmo_plugin);

    node_gizmo_plugin.instantiate();
    add_node_3d_gizmo_plugin(node_gizmo_plugin);

    add_tool_menu_item("Refresh OpenAL Devices", callable_mp(this, &VAConversionPlugin::refresh_output_device_setting));
}

void VAConversionPlugin::_exit_tree()
{
    remove_tool_menu_item("Refresh OpenAL Devices");

    remove_context_menu_plugin(context_menu_plugin);
    context_menu_plugin.unref();

    remove_inspector_plugin(device_refresh_inspector_plugin);
    device_refresh_inspector_plugin.unref();

    remove_inspector_plugin(material_inspector_plugin);
    material_inspector_plugin.unref();

    remove_inspector_plugin(material_properties_inspector_plugin);
    material_properties_inspector_plugin.unref();

    remove_node_3d_gizmo_plugin(world_gizmo_plugin);
    world_gizmo_plugin.unref();

    remove_node_3d_gizmo_plugin(node_gizmo_plugin);
    node_gizmo_plugin.unref();

    if (Engine::get_singleton()->has_singleton(DEBUGGER_BRIDGE_SINGLETON_NAME))
        Engine::get_singleton()->unregister_singleton(DEBUGGER_BRIDGE_SINGLETON_NAME);

    if (debugger_bridge)
    {
        memdelete(debugger_bridge);
        debugger_bridge = nullptr;
    }

    remove_debugger_plugin(debugger_plugin);
    debugger_plugin.unref();
}
