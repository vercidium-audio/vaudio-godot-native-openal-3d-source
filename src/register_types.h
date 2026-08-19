#pragma once

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level);
void uninitialize_vaudio_godot_native_openal_3d_module(ModuleInitializationLevel p_level);

// Rebuilds audio/vaudio/output_device's Project Settings dropdown from
// ALManager::get_available_devices() - see the definition in register_types.cpp for why an
// already-open Project Settings dialog needs its Output Device row redrawn to see the result.
void refresh_output_device_hint();
