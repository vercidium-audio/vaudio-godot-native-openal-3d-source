#include "va_debugger_plugin.h"

#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace va_godot;

void VADebuggerPlugin::_bind_methods()
{
}

void VADebuggerPlugin::sync_primitive(const String &scene_root_name, const NodePath &node_path, const String &material, const Variant &use_flat_transmission)
{
    Array sessions = get_sessions();

    for (int i = 0; i < sessions.size(); i++)
    {
        Ref<EditorDebuggerSession> session = sessions[i];

        // Only sessions currently attached to a running game can receive messages - a session
        // stays in get_sessions() after the game it was attached to has stopped.
        if (session.is_valid() && session->is_active())
        {
            Array data;
            data.push_back(scene_root_name);
            data.push_back(node_path);
            data.push_back(material);
            data.push_back(use_flat_transmission);
            session->send_message("vaudio:sync_primitive", data);
        }
    }
}
