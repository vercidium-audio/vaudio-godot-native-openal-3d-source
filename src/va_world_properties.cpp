#include "va_conversions.h"
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
    vaWorldSetPosition(world, ToVAudio(value));
}

void VAWorld::set_size(Vector3 value)
{
    size = value;
    vaWorldSetSize(world, ToVAudio(value));
}

void VAWorld::set_epsilon(float value)
{
    epsilon = value;
    vaWorldSetEpsilon(world, value);
}

void VAWorld::set_world_is_indoors(bool value)
{
    world_is_indoors = value;
    vaWorldSetWorldIsIndoors(world, value);
}

void VAWorld::set_maximum_grouped_eax_count(int value)
{
    maximum_grouped_eax_count = std::max(0, value);
    vaWorldSetMaximumGroupedEAXCount(world, maximum_grouped_eax_count);
}

void VAWorld::set_meters_per_unit(float value)
{
    meters_per_unit = std::max(0.0001f, value);
    vaWorldSetMetersPerUnit(world, meters_per_unit);
}

void VAWorld::set_speed_of_sound(float value)
{
    speed_of_sound = std::max(0.0001f, value);
    vaWorldSetInverseSpeedOfSound(world, 1.0f / speed_of_sound);
}

void VAWorld::set_humidity(float value)
{
    humidity = value;
    vaWorldSetAirAbsorptionHumidity(world, value);
}

void VAWorld::set_temperature(float value)
{
    temperature = value;
    vaWorldSetAirAbsorptionTemperature(world, value);
}

void VAWorld::set_pressure(float value)
{
    pressure = value;
    vaWorldSetAirAbsorptionPressure(world, value);
}

void VAWorld::set_reference_frequency_lf(float value)
{
    reference_frequency_lf = std::max(0.0001f, value);
    vaWorldSetReferenceFrequencyLF(world, reference_frequency_lf);
}

void VAWorld::set_reference_frequency_hf(float value)
{
    reference_frequency_hf = std::max(0.0001f, value);
    vaWorldSetReferenceFrequencyHF(world, reference_frequency_hf);
}

void VAWorld::set_emitters_outside_the_world_are_muffled(bool value)
{
    emitters_outside_the_world_are_muffled = value;
    vaWorldSetEmittersOutsideTheWorldAreMuffled(world, value);
}

void VAWorld::set_maximum_concurrency_level(int value)
{
    maximum_concurrency_level = std::max(1, value);
    vaWorldSetMaximumConcurrencyLevel(world, maximum_concurrency_level);
}

void VAWorld::set_work_item_count(int value)
{
    work_item_count = std::max(0, value);
    vaWorldSetWorkItemCount(world, work_item_count);
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
        UtilityFunctions::push_error(
            "[vaudio-godot-native-openal] VAWorld::export_to_file failed for '", file_path, "' (VAResult=", (int)result, ")");
        return false;
    }

    return true;
}

} // namespace va_godot
