#include "openal/al_manager.h"
#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

namespace va_godot
{

void VAWorld::set_position(const Vector3 &value)
{
    Node3D::set_position(value);
    update_gizmos();

    if (!world)
        return;

    VAResult result = vaWorldSetPosition(world, ToVAudio(value));

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world position (may be NaN/Infinity)");
}

void VAWorld::set_bounds_size(Vector3 value)
{
    bounds_size = value;
    update_gizmos();

    if (!world)
        return;

    VAResult result = vaWorldSetSize(world, ToVAudio(value));

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world size (may be negative or NaN/Infinity)");
}

void VAWorld::set_bounds_color(Color value)
{
    bounds_color = value;
    update_gizmos();
}

void VAWorld::set_epsilon(float value)
{
    epsilon = value;

    if (!world)
        return;

    VAResult result = vaWorldSetEpsilon(world, value);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world epsilon (may be NaN/Infinity)");
}

void VAWorld::set_world_is_indoors(bool value)
{
    world_is_indoors = value;

    if (!world)
        return;

    // No need to check result - world is defined
    vaWorldSetWorldIsIndoors(world, value);
}

void VAWorld::set_maximum_grouped_eax_count(int value)
{
    maximum_grouped_eax_count = std::max(0, value);

    if (!world)
        return;

    VAResult result = vaWorldSetMaximumGroupedEAXCount(world, maximum_grouped_eax_count);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world maximum grouped EAX count (may be < 1)");
}

void VAWorld::set_meters_per_unit(float value)
{
    meters_per_unit = std::max(0.0001f, value);

    if (world)
    {
        VAResult result = vaWorldSetMetersPerUnit(world, meters_per_unit);

        if (result != VA_SUCCESS)
            VA_ERROR_NAMED_RESULT(result, "Failed to set world meters per unit (may be <= 0, NaN or Infinity)");
    }

    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_meters_per_unit(meters_per_unit);
}

void VAWorld::set_speed_of_sound(float value)
{
    speed_of_sound = std::max(0.0001f, value);

    if (world)
    {
        VAResult result = vaWorldSetInverseSpeedOfSound(world, 1.0f / speed_of_sound);

        if (result != VA_SUCCESS)
            VA_ERROR_NAMED_RESULT(result, "Failed to set world speed of sound (may be <= 0, NaN or Infinity)");
    }

    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_speed_of_sound(speed_of_sound);
}

void VAWorld::set_master_volume(float value)
{
    master_volume = value;

    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_master_volume(master_volume);
}

void VAWorld::set_distance_model(int value)
{
    distance_model = value;

    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_distance_model(static_cast<ALenum>(distance_model));
}

void VAWorld::set_reverb_only(bool value)
{
    reverb_only = value;

    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_reverb_only(reverb_only);
}

void VAWorld::set_humidity(float value)
{
    humidity = value;

    if (!world)
        return;

    VAResult result = vaWorldSetAirAbsorptionHumidity(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world humidity (may be < 0, > 1, NaN or Infinity)");
}

void VAWorld::set_temperature(float value)
{
    temperature = value;

    if (!world)
        return;

    VAResult result = vaWorldSetAirAbsorptionTemperature(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world temperature (may be <= -273.15, NaN or Infinity)");
}

void VAWorld::set_pressure(float value)
{
    pressure = value;

    if (!world)
        return;

    VAResult result = vaWorldSetAirAbsorptionPressure(world, value);

    if (result != VA_SUCCESS && result != VA_UNCHANGED)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world pressure (may be <= 0, NaN or Infinity)");
}

void VAWorld::set_reference_frequency_lf(float value)
{
    if (!world)
        return;

    VAResult result = vaWorldSetReferenceFrequencyLF(world, reference_frequency_lf);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world LF reference frequency (may be <= 0, NaN or Infinity)");
}

void VAWorld::set_reference_frequency_hf(float value)
{
    if (!world)
        return;

    VAResult result = vaWorldSetReferenceFrequencyHF(world, reference_frequency_hf);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world HF reference frequency (may be <= 0, NaN or Infinity)");
}

void VAWorld::set_emitters_outside_the_world_are_muffled(bool value)
{
    emitters_outside_the_world_are_muffled = value;

    if (!world)
        return;

    // No need to check result
    vaWorldSetEmittersOutsideTheWorldAreMuffled(world, value);
}

void VAWorld::set_maximum_concurrency_level(int value)
{
    maximum_concurrency_level = std::max(0, value);

    if (!world)
        return;

    // 0 maps to processor count - 1
    if (value == 0)
        value = std::max(1, OS::get_singleton()->get_processor_count() - 1);

    VAResult result = vaWorldSetMaximumConcurrencyLevel(world, value);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world maximum concurrency level (may be < 1)");
}

void VAWorld::set_work_item_count(int value)
{
    // Editor field has a range hint of 1+, but callers can still pass 0 or below via code.
    work_item_count = std::max(0, value);

    if (!world)
        return;

    VAResult result = vaWorldSetWorkItemCount(world, work_item_count);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world work item count (may be < 1)");
}

double VAWorld::get_main_thread_time() const
{
    return vaWorldGetMainThreadTime(world);
}

double VAWorld::get_preparation_time() const
{
    return vaWorldGetPreparationTime(world);
}

double VAWorld::get_raytracing_time() const
{
    return vaWorldGetRaytracingTime(world);
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
        VA_ERROR_NAMED_RESULT(result, "Failed to export the world to '", file_path, "'");
        return false;
    }

    return true;
}

} // namespace va_godot
