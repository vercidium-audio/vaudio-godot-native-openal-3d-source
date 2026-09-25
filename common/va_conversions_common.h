#pragma once

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

inline VAColor ToVAudio(const Color &c)
{
    return VAColor{(unsigned char)(c.r * 255.0f), (unsigned char)(c.g * 255.0f), (unsigned char)(c.b * 255.0f), (unsigned char)(c.a * 255.0f)};
}

inline Color FromVAudio(const VAColor &c)
{
    return Color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

// Convert VAResults to their string name
inline String VAResultToString(VAResult result)
{
    switch (result)
    {
        case VA_SUCCESS: return "VA_SUCCESS";
        case VA_INVALID_VALUE: return "VA_INVALID_VALUE";
        case VA_OUT_OF_RANGE: return "VA_OUT_OF_RANGE";
        case VA_ALREADY_EXISTS: return "VA_ALREADY_EXISTS";
        case VA_FEATURE_DISABLED: return "VA_FEATURE_DISABLED";
        case VA_ERROR_IN_USE: return "VA_ERROR_IN_USE";
        case VA_INVALID_COUNT: return "VA_INVALID_COUNT";
        case VA_WORLD_CONFLICT: return "VA_WORLD_CONFLICT";
        case VA_ERROR_FILE_OPEN: return "VA_ERROR_FILE_OPEN";
        case VA_ERROR_FILE_WRITE: return "VA_ERROR_FILE_WRITE";
        case VA_ERROR_FILE_VERSION: return "VA_ERROR_FILE_VERSION";
        case VA_ERROR_FILE_CORRUPT: return "VA_ERROR_FILE_CORRUPT";
        case VA_INVALID_MATERIAL: return "VA_INVALID_MATERIAL";
        case VA_MATERIAL_DOES_NOT_EXIST: return "VA_MATERIAL_DOES_NOT_EXIST";
        case VA_NOT_ADDED_TO_WORLD: return "VA_NOT_ADDED_TO_WORLD";
        case VA_INSUFFICIENT_VERTICES: return "VA_INSUFFICIENT_VERTICES";
        case VA_INVALID_VERTEX_COUNT: return "VA_INVALID_VERTEX_COUNT";
        case VA_INVALID_ARRAY: return "VA_INVALID_ARRAY";
        case VA_INVALID_POINTER: return "VA_INVALID_POINTER";
        case VA_UNCHANGED: return "VA_UNCHANGED";
        case VA_NOT_FOUND: return "VA_NOT_FOUND";
        case VA_STILL_RUNNING: return "VA_STILL_RUNNING";
        case VA_PENDING_REMOVAL: return "VA_PENDING_REMOVAL";
        case VA_CONFIG_ERROR: return "VA_CONFIG_ERROR";
        case VA_TRUE: return "VA_TRUE";
        case VA_FALSE: return "VA_FALSE";
        case VA_WRONG_DIMENSION: return "VA_WRONG_DIMENSION";
        case VA_MISSING_MATERIAL_CALLBACK: return "VA_MISSING_MATERIAL_CALLBACK";
        default: return "UNKNOWN(" + String::num_int64(result) + ")";
    }
}
