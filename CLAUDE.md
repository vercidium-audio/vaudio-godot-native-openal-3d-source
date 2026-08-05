# Overview

This repo is a native Godot GDExtension (C++) plugin wrapping the vaudio C SDK, with OpenAL
Soft as the audio backend, for non-Mono ("Native Godot") projects.

This plugin links against the package vaudionative C DLL, not the source.

# How to use

To build, run build.bat.

# C++ code style

- Allman brace style: opening curly braces go on their own new line (for functions, classes,
  control flow, everything), not K&R/"same line" style.
- 4 spaces per indent level, not tabs.
- Don't split comments across lines, keep it all on one line

Don't use braces if it'll just be one line inside the braces, i.e:

```
// Don't do this
if (arrays.is_empty())
{
    continue;
}

// Do this
if (arrays.is_empty())
    continue;

// Except if it's an assignment
if (arrays.is_empty())
{
    arrays = xyz;
}
```

# Logging

When logging info / warning / errors, use the NAMED functions in src/va_engine_util.h:

```c
#define VA_LOG_NAMED(...) (godot::UtilityFunctions::print(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
#define VA_WARN_NAMED(...) (godot::UtilityFunctions::push_warning(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
#define VA_ERROR_NAMED(...) (godot::UtilityFunctions::push_error(VA_LOG_TAG, get_name(), ": ", __VA_ARGS__))
```

If the code is not inside a Node class, use the non-NAMED versions:

```c
#define VA_LOG(...) (godot::UtilityFunctions::print(VA_LOG_TAG, __VA_ARGS__))
#define VA_WARN(...) (godot::UtilityFunctions::push_warning(VA_LOG_TAG, __VA_ARGS__))
#define VA_ERROR(...) (godot::UtilityFunctions::push_error(VA_LOG_TAG, __VA_ARGS__))
```

# Continuous Cleanup

While working, if you notice unclean logs, fix them:
- Use the NAMED variants above if possible
- Don't log internal function names like `VAWorld::set_size` instead make the error nice and human readable, like `Failed to set size` instead of `VAWorld::set_size failed`. Remember the user isn't working with this code, they are working from the Godot editor
- Many logs repeat this text: `(VAResult=", VAResultToString(result), ")"`, maybe add a new macro for VA_ERROR_RESULT and VA_ERROR_NAMED_RESULT that also takes the VAResult as a parameter and automatically appends it in the log.

# References

vaudio.h lives at `thirdparty/vaudio/include/vaudio.h`

See user_context.md for more info (may not exist, this file is git-ignored)