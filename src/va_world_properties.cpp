#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

// VAWorldProperties.cs port. Only pending_shutdown's bind/get/set live in
// va_world.cpp/_bind_methods (it was already there before this file existed);
// everything else the C# exposes under [ExportGroup] is added here. Each
// setter clamps/validates the same way its C# counterpart does before
// forwarding to the native handle, which always exists by the time these run
// (world is created in VAWorld::VAWorld() itself - see va_world.cpp).

namespace va_godot
{

void VAWorld::set_position(Vector3 value)
{
    position = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetPosition(world, ToVAudio(value));

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_position failed (value contains NaN/Infinity) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_size(Vector3 value)
{
    size = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetSize(world, ToVAudio(value));

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_size failed (value contains NaN/Infinity, or a component is negative) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_epsilon(float value)
{
    epsilon = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetEpsilon(world, value);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_epsilon failed (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_world_is_indoors(bool value)
{
    world_is_indoors = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetWorldIsIndoors(world, value);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_world_is_indoors failed (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_maximum_grouped_eax_count(int value)
{
    // SDK requires >= 1 (VA_OUT_OF_RANGE otherwise); only the negative side is clamped here,
    // so a caller passing 0 will still be rejected by the SDK and reported below.
    maximum_grouped_eax_count = std::max(0, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetMaximumGroupedEAXCount(world, maximum_grouped_eax_count);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_maximum_grouped_eax_count failed for value ", maximum_grouped_eax_count,
            " (must be >= 1) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_meters_per_unit(float value)
{
    meters_per_unit = std::max(0.0001f, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetMetersPerUnit(world, meters_per_unit);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_meters_per_unit failed (value is NaN/Infinity) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_speed_of_sound(float value)
{
    speed_of_sound = std::max(0.0001f, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetInverseSpeedOfSound(world, 1.0f / speed_of_sound);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_speed_of_sound failed (inverse speed is NaN/Infinity) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_humidity(float value)
{
    humidity = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetAirAbsorptionHumidity(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
    {
        VA_ERROR(
            "VAWorld::set_humidity failed for value ", value,
            " (must be between 0 and 1) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_temperature(float value)
{
    temperature = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetAirAbsorptionTemperature(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
    {
        VA_ERROR(
            "VAWorld::set_temperature failed for value ", value,
            " (must be > -273.15) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_pressure(float value)
{
    pressure = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetAirAbsorptionPressure(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
    {
        VA_ERROR(
            "VAWorld::set_pressure failed for value ", value,
            " (must be > 0) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_reference_frequency_lf(float value)
{
    reference_frequency_lf = std::max(0.0001f, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetReferenceFrequencyLF(world, reference_frequency_lf);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_reference_frequency_lf failed (value is NaN/Infinity) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_reference_frequency_hf(float value)
{
    reference_frequency_hf = std::max(0.0001f, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetReferenceFrequencyHF(world, reference_frequency_hf);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_reference_frequency_hf failed (value is NaN/Infinity) (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_emitters_outside_the_world_are_muffled(bool value)
{
    emitters_outside_the_world_are_muffled = value;

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetEmittersOutsideTheWorldAreMuffled(world, value);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_emitters_outside_the_world_are_muffled failed (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_maximum_concurrency_level(int value)
{
    maximum_concurrency_level = std::max(1, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetMaximumConcurrencyLevel(world, maximum_concurrency_level);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_maximum_concurrency_level failed (VAResult=", VAResultToString(result), ")");
    }
}

void VAWorld::set_work_item_count(int value)
{
    // SDK requires >= 1 (VA_OUT_OF_RANGE otherwise); only the negative side is clamped here,
    // so a caller passing 0 will still be rejected by the SDK and reported below.
    work_item_count = std::max(0, value);

    if (!world)
    {
        return;
    }

    VAResult result = vaWorldSetWorkItemCount(world, work_item_count);

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::set_work_item_count failed for value ", work_item_count,
            " (must be >= 1) (VAResult=", VAResultToString(result), ")");
    }
}

double VAWorld::get_main_thread_time() const
{
    return vaWorldGetMainThreadTime(world);
}

double VAWorld::get_raytracing_time() const
{
    return vaWorldGetRaytracingTime(world);
}

double VAWorld::get_preparation_time() const
{
    return vaWorldGetPreparationTime(world);
}

double VAWorld::get_analysis_time() const
{
    return vaWorldGetAnalysisTime(world);
}

int VAWorld::get_grouped_eax_count() const
{
    return vaWorldGetGroupedEAXCount(world);
}

float VAWorld::get_grouped_eax_gain_lf(int index) const
{
    return vaWorldGetGroupedEAX(world)[index]->gainLF;
}

float VAWorld::get_grouped_eax_gain_hf(int index) const
{
    return vaWorldGetGroupedEAX(world)[index]->gainHF;
}

float VAWorld::get_grouped_eax_decay_time(int index) const
{
    return vaWorldGetGroupedEAX(world)[index]->decayTime;
}

bool VAWorld::export_to_file(const String &file_path)
{
    VAResult result = vaWorldExport(world, file_path.utf8().get_data());

    if (result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::export_to_file failed for '", file_path, "' (VAResult=", VAResultToString(result), ")");
        return false;
    }

    return true;
}

} // namespace va_godot
