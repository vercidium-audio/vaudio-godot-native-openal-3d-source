#include "openal/al_manager.h"
#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"

#include <godot_cpp/classes/os.hpp>
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

// Shadows the inherited Node3D::set_position (see _bind_methods) so that moving this node - via
// the viewport gizmo, the Inspector's Position field, or code - also updates vaWorldSetPosition.
// The bounds are always axis-aligned starting at this position (see _validate_property, which
// hides rotation/scale), so the node's position doubles as the AABB's world-space origin.
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

// Editor-only visualization setting, not forwarded to the SDK - just recolors the viewport gizmo.
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
    // Editor field has a range hint of 1+, but callers can still pass 0 or below via code.
    // SDK requires >= 1 (VA_OUT_OF_RANGE otherwise); only the negative side is clamped here,
    // so a caller passing 0 will still be rejected by the SDK and reported below.
    maximum_grouped_eax_count = std::max(0, value);

    if (!world)
        return;

    VAResult result = vaWorldSetMaximumGroupedEAXCount(world, maximum_grouped_eax_count);

    if (result != VA_SUCCESS)
        VA_ERROR_NAMED_RESULT(result, "Failed to set world maximum grouped EAX count (may be < 1)");
}

void VAWorld::set_meters_per_unit(float value)
{
    // Matches the editor range hint's minimum; also guards values set via code, which bypass that hint.
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
    // Matches the editor range hint's minimum; also guards values set via code, which bypass that hint.
    speed_of_sound = std::max(0.0001f, value);

    if (world)
    {
        VAResult result = vaWorldSetInverseSpeedOfSound(world, 1.0f / speed_of_sound);

        if (result != VA_SUCCESS)
            VA_ERROR_NAMED_RESULT(result, "Failed to set world speed of sound (may be <= 0, NaN or Infinity)");
    }

    // AL's set_speed_of_sound takes the direct value, not the inverse that vaWorldSetInverseSpeedOfSound takes.
    if (ALManager::get_singleton())
        ALManager::get_singleton()->set_speed_of_sound(speed_of_sound);
}

// AL-only settings (no vaudio SDK equivalent) - forwarded straight to ALManager. Guarded since
// VAWorld can exist before/without ALManager having initialized successfully (e.g. in the editor).
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
