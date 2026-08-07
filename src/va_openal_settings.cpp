#include "va_openal_settings.h"

#include <godot_cpp/core/class_db.hpp>

#include "openal/al_manager.h"

namespace va_godot
{

void VAOpenALSettings::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_device_name"), &VAOpenALSettings::get_device_name);
    ClassDB::bind_method(D_METHOD("set_device_name", "value"), &VAOpenALSettings::set_device_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_name"), "set_device_name", "get_device_name");

    ClassDB::bind_method(D_METHOD("get_max_reverb_sends"), &VAOpenALSettings::get_max_reverb_sends);
    ClassDB::bind_method(D_METHOD("set_max_reverb_sends", "value"), &VAOpenALSettings::set_max_reverb_sends);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_reverb_sends", PROPERTY_HINT_RANGE, "0,16,or_greater"), "set_max_reverb_sends", "get_max_reverb_sends");

    ClassDB::bind_method(D_METHOD("get_master_volume"), &VAOpenALSettings::get_master_volume);
    ClassDB::bind_method(D_METHOD("set_master_volume", "value"), &VAOpenALSettings::set_master_volume);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "master_volume", PROPERTY_HINT_RANGE, "0.0,2.0,0.01,or_greater"), "set_master_volume", "get_master_volume");

    ClassDB::bind_method(D_METHOD("get_sample_rate"), &VAOpenALSettings::get_sample_rate);
    ClassDB::bind_method(D_METHOD("set_sample_rate", "value"), &VAOpenALSettings::set_sample_rate);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_rate", PROPERTY_HINT_ENUM, "Driver Default:0,22050,44100,48000,96000"), "set_sample_rate", "get_sample_rate");

    ClassDB::bind_method(D_METHOD("get_hrtf_enabled"), &VAOpenALSettings::get_hrtf_enabled);
    ClassDB::bind_method(D_METHOD("set_hrtf_enabled", "value"), &VAOpenALSettings::set_hrtf_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hrtf_enabled"), "set_hrtf_enabled", "get_hrtf_enabled");

    ClassDB::bind_method(D_METHOD("get_distance_model"), &VAOpenALSettings::get_distance_model);
    ClassDB::bind_method(D_METHOD("set_distance_model", "value"), &VAOpenALSettings::set_distance_model);
    // Enum hint values are OpenAL's own AL_*_DISTANCE(_CLAMPED) constants (AL/al.h), spelled out
    // literally here since PROPERTY_HINT_ENUM's "Name:value" syntax needs a compile-time string.
    ADD_PROPERTY(PropertyInfo(Variant::INT, "distance_model", PROPERTY_HINT_ENUM,
        "Inverse Distance Clamped:53250,Inverse Distance:53249,Linear Distance Clamped:53252,"
        "Linear Distance:53251,Exponent Distance Clamped:53254,Exponent Distance:53253,None:0"),
        "set_distance_model", "get_distance_model");

    ClassDB::bind_method(D_METHOD("get_meters_per_unit"), &VAOpenALSettings::get_meters_per_unit);
    ClassDB::bind_method(D_METHOD("set_meters_per_unit", "value"), &VAOpenALSettings::set_meters_per_unit);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "meters_per_unit", PROPERTY_HINT_RANGE, "0.0001,10.0,0.0001,or_greater"), "set_meters_per_unit", "get_meters_per_unit");

    ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &VAOpenALSettings::get_speed_of_sound);
    ClassDB::bind_method(D_METHOD("set_speed_of_sound", "value"), &VAOpenALSettings::set_speed_of_sound);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1,or_greater"), "set_speed_of_sound", "get_speed_of_sound");

    ClassDB::bind_method(D_METHOD("get_reverb_only"), &VAOpenALSettings::get_reverb_only);
    ClassDB::bind_method(D_METHOD("set_reverb_only", "value"), &VAOpenALSettings::set_reverb_only);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverb_only"), "set_reverb_only", "get_reverb_only");

    ClassDB::bind_method(D_METHOD("get_available_devices"), &VAOpenALSettings::get_available_devices);

    ClassDB::bind_method(D_METHOD("refresh_devices"), &VAOpenALSettings::refresh_devices);
}

// Turns device_name into an editor dropdown of the driver's currently available
// playback devices, without changing how the value itself is stored (still a
// plain String, set via set_device_name like any other property) - a
// suggestion hint rather than a strict enum, since the device list is only
// known at runtime and can vary between machines/driver sessions: a strict
// PROPERTY_HINT_ENUM would reject or blank out a saved device_name that isn't
// in the list currently being queried (e.g. project opened before drivers
// finish enumerating, or the device is temporarily unplugged).
void VAOpenALSettings::_validate_property(PropertyInfo &property) const
{
    if (property.name != StringName("device_name"))
        return;

    ALManager *manager = ALManager::get_singleton();

    if (!manager)
        return;

    PackedStringArray devices = manager->get_available_devices();

    if (devices.is_empty())
        return;

    property.hint = PROPERTY_HINT_ENUM_SUGGESTION;
    property.hint_string = String(",").join(devices);
}

void VAOpenALSettings::_ready()
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager)
    {
        return;
    }

    manager->reinitialize(device_name, max_reverb_sends, sample_rate, hrtf_enabled);
    manager->set_master_volume(master_volume);
    manager->set_distance_model(static_cast<ALenum>(distance_model));
    manager->set_meters_per_unit(meters_per_unit);
    manager->set_speed_of_sound(speed_of_sound);
    manager->set_reverb_only(reverb_only);
}

String VAOpenALSettings::get_device_name() const
{
    return device_name;
}

void VAOpenALSettings::set_device_name(const String &value)
{
    device_name = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->reinitialize(device_name, max_reverb_sends, sample_rate, hrtf_enabled);
    }
}

int VAOpenALSettings::get_max_reverb_sends() const
{
    return max_reverb_sends;
}

void VAOpenALSettings::set_max_reverb_sends(int value)
{
    max_reverb_sends = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->reinitialize(device_name, max_reverb_sends, sample_rate, hrtf_enabled);
    }
}

float VAOpenALSettings::get_master_volume() const
{
    return master_volume;
}

void VAOpenALSettings::set_master_volume(float value)
{
    master_volume = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->set_master_volume(value);
    }
}

int VAOpenALSettings::get_sample_rate() const
{
    return sample_rate;
}

void VAOpenALSettings::set_sample_rate(int value)
{
    sample_rate = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->reinitialize(device_name, max_reverb_sends, sample_rate, hrtf_enabled);
    }
}

bool VAOpenALSettings::get_hrtf_enabled() const
{
    return hrtf_enabled;
}

void VAOpenALSettings::set_hrtf_enabled(bool value)
{
    hrtf_enabled = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->reinitialize(device_name, max_reverb_sends, sample_rate, hrtf_enabled);
    }
}

int VAOpenALSettings::get_distance_model() const
{
    return distance_model;
}

void VAOpenALSettings::set_distance_model(int value)
{
    distance_model = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->set_distance_model(static_cast<ALenum>(distance_model));
    }
}

float VAOpenALSettings::get_meters_per_unit() const
{
    return meters_per_unit;
}

void VAOpenALSettings::set_meters_per_unit(float value)
{
    meters_per_unit = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->set_meters_per_unit(value);
    }
}

float VAOpenALSettings::get_speed_of_sound() const
{
    return speed_of_sound;
}

void VAOpenALSettings::set_speed_of_sound(float value)
{
    speed_of_sound = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->set_speed_of_sound(value);
    }
}

bool VAOpenALSettings::get_reverb_only() const
{
    return reverb_only;
}

void VAOpenALSettings::set_reverb_only(bool value)
{
    reverb_only = value;

    ALManager *manager = ALManager::get_singleton();

    if (manager)
    {
        manager->set_reverb_only(value);
    }
}

PackedStringArray VAOpenALSettings::get_available_devices() const
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager)
    {
        return PackedStringArray();
    }

    return manager->get_available_devices();
}

void VAOpenALSettings::refresh_devices()
{
    notify_property_list_changed();
}

} // namespace va_godot
