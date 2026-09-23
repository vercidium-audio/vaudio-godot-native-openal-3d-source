#include "va_debugger_plugin.h"

#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace va_godot;

void VADebuggerPlugin::_bind_methods()
{
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
