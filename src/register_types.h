#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

void initialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level);
void uninitialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level);

// Rebuilds audio/vaudio/output_device's Project Settings dropdown from ALManager::get_available_devices() - see the definition in register_types.cpp.
void refresh_output_device_hint();

// Receiving end of VADebuggerPlugin::sync_viewport_camera - defined per-repo in register_types.cpp since the camera-sync payload shape/native calls differ per dimension. Declared here (rather than static) so the shared on_debugger_message in common/register_types_common.cpp can dispatch to it.
bool on_sync_viewport_camera(const Array &data);

// Depth-first search for a descendant named scene_root_name - used instead of SceneTree::get_current_scene(), which isn't
// reliable in a game that manually adds a scene as a plain child rather than via change_scene_to_*/change_scene_to_file.
// Defined once in common/register_types_common.cpp since it's dimension-agnostic; declared here (not static) so it's visible from that TU.
Node *find_child_named_recursive(Node *node, const StringName &name);

// Dimension-agnostic EngineDebugger message dispatcher (sync_material_properties/sync_viewport_camera/sync_primitive), registered as the "vaudio" message capture in initialize_*_module. Defined once in common/register_types_common.cpp.
bool on_debugger_message(const String &message, const Array &data);

// Dimension-agnostic Project Settings registration (audio/vaudio/*). Defined once in common/register_types_common.cpp.
void register_project_settings();
