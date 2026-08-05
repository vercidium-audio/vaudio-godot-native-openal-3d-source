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

    ClassDB::bind_method(D_METHOD("get_available_devices"), &VAOpenALSettings::get_available_devices);
}

void VAOpenALSettings::_ready()
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager)
    {
        return;
    }

    manager->reinitialize(device_name, max_reverb_sends);
    manager->set_master_volume(master_volume);
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
        manager->reinitialize(device_name, max_reverb_sends);
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
        manager->reinitialize(device_name, max_reverb_sends);
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

PackedStringArray VAOpenALSettings::get_available_devices() const
{
    ALManager *manager = ALManager::get_singleton();

    if (!manager)
    {
        return PackedStringArray();
    }

    return manager->get_available_devices();
}

} // namespace va_godot
