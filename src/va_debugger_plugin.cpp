#include "va_debugger_plugin.h"

#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace va_godot;

void VADebuggerPlugin::_bind_methods()
{
}

void VADebuggerPlugin::sync_primitive(const String &scene_root_name, const NodePath &node_path, const String &material,
    const Variant &use_flat_transmission, const String &propagate, const Variant &propagate_layer)
{
    Array sessions = get_sessions();

    for (int i = 0; i < sessions.size(); i++)
    {
        Ref<EditorDebuggerSession> session = sessions[i];

        // A session stays in get_sessions() after the game it was attached to has stopped, so check is_active() too.
        if (session.is_valid() && session->is_active())
        {
            Array data;
            data.push_back(scene_root_name);
            data.push_back(node_path);
            data.push_back(material);
            data.push_back(use_flat_transmission);
            data.push_back(propagate);
            data.push_back(propagate_layer);
            session->send_message("vaudio:sync_primitive", data);
        }
    }
}

void VADebuggerPlugin::sync_material_properties(const String &scene_root_name, const NodePath &node_path, const String &node_name,
    bool is_custom_material, int material_type, const String &custom_material_name, float absorption_lf,
    float absorption_hf, float scattering, float transmission_lf, float transmission_hf,
    float flat_transmission_lf, float flat_transmission_hf, const Color &color)
{
    Array sessions = get_sessions();

    for (int i = 0; i < sessions.size(); i++)
    {
        Ref<EditorDebuggerSession> session = sessions[i];

        if (session.is_valid() && session->is_active())
        {
            Array data;
            data.push_back(scene_root_name);
            data.push_back(node_path);
            data.push_back(node_name);
            data.push_back(is_custom_material);
            data.push_back(material_type);
            data.push_back(custom_material_name);
            data.push_back(absorption_lf);
            data.push_back(absorption_hf);
            data.push_back(scattering);
            data.push_back(transmission_lf);
            data.push_back(transmission_hf);
            data.push_back(flat_transmission_lf);
            data.push_back(flat_transmission_hf);
            data.push_back(color);
            session->send_message("vaudio:sync_material_properties", data);
        }
    }
}

void VADebuggerPlugin::sync_viewport_camera(const Vector3 &position, const Vector3 &rotation, float fov_degrees)
{
    Array sessions = get_sessions();

    for (int i = 0; i < sessions.size(); i++)
    {
        Ref<EditorDebuggerSession> session = sessions[i];

        if (session.is_valid() && session->is_active())
        {
            Array data;
            data.push_back(position);
            data.push_back(rotation);
            data.push_back(fov_degrees);
            session->send_message("vaudio:sync_viewport_camera", data);
        }
    }
}
