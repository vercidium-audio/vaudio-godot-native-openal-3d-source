#pragma once

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Shorthand for the Engine::get_singleton()->is_editor_hint() check repeated across the plugin.
#define IS_EDITOR_HINT() (godot::Engine::get_singleton()->is_editor_hint())

// Plugin-wide log tag, prepended by VA_LOG/VA_WARN/VA_ERROR so it doesn't need to be retyped at every call site.
#define VA_LOG_TAG "[vaudio-godot-native-openal] "

// Logging macros so call sites don't have to retype the "[vaudio-godot-native-openal] " tag.
// Args are forwarded as-is (Godot's print/push_warning/push_error take a comma-separated arg list, not a format
// string), so callers still pass their own name/context args, e.g. VA_ERROR(get_name(), ": failed to decode").
#define VA_LOG(...) (godot::UtilityFunctions::print(VA_LOG_TAG, __VA_ARGS__))
#define VA_WARN(...) (godot::UtilityFunctions::push_warning(VA_LOG_TAG, __VA_ARGS__))
#define VA_ERROR(...) (godot::UtilityFunctions::push_error(VA_LOG_TAG, __VA_ARGS__))
