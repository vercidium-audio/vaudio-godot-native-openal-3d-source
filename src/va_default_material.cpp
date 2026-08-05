#include "va_default_material.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

#include <godot_cpp/variant/utility_functions.hpp>

// Dropdown order must match VAMaterialType's declaration order in vaudio.h
static const char *DEFAULT_MATERIAL_NAME_HINT =
    "Air,Brick,Cloth,Concrete,Concrete Polished,Dirt,Glass,Grass,Gravel,"
    "Gyprock,Ice,Leaf,Marble,Metal,Mud,Rock,Sand,Snow,Tile,Tree,Water,"
    "Wood Indoor,Wood Outdoor";

namespace
{
    // Convert VAMaterialType to their string name for error/warning logging
    String VAMaterialTypeToString(VAMaterialType material_type)
    {
        static const PackedStringArray names = String(DEFAULT_MATERIAL_NAME_HINT).split(",");

        if (material_type < 0 || material_type >= names.size())
            return "UNKNOWN(" + String::num_int64(material_type) + ")";

        return names[material_type];
    }

    struct MaterialDefaults
    {
        float absorption_lf;
        float absorption_hf;
        float scattering;
        float transmission_lf;
        float transmission_hf;
        float plane_transmission_lf;
        float plane_transmission_hf;
        Color color;
    };

    // Cache default material values
    const MaterialDefaults &get_material_defaults(VAMaterialType material_type)
    {
        static MaterialDefaults defaults[VAMaterialTypeCount];
        static bool cached = false;

        if (!cached)
        {
            ::VAWorld *world = vaWorldCreate();

            for (int i = 0; i < VAMaterialTypeCount; i++)
            {
                defaults[i].absorption_lf = vaWorldGetMaterialAbsorptionLF(world, i);
                defaults[i].absorption_hf = vaWorldGetMaterialAbsorptionHF(world, i);
                defaults[i].scattering = vaWorldGetMaterialScattering(world, i);
                defaults[i].transmission_lf = vaWorldGetMaterialTransmissionLF(world, i);
                defaults[i].transmission_hf = vaWorldGetMaterialTransmissionHF(world, i);
                defaults[i].plane_transmission_lf = vaWorldGetMaterialPlaneTransmissionLF(world, i);
                defaults[i].plane_transmission_hf = vaWorldGetMaterialPlaneTransmissionHF(world, i);
                defaults[i].color = FromVAudio(vaWorldGetMaterialColor(world, i));
            }

            VAResult destroy_result = vaWorldDestroy(world);
            
            if (destroy_result != VA_SUCCESS)
                VA_ERROR_RESULT(destroy_result, "Failed to destroy scratch world when extracting default materials");

            cached = true;
        }

        return defaults[material_type];
    }

    // Log VA_INVALID_VALUE/VA_OUT_OF_RANGE errors
    void log_if_material_setter_failed(VAResult result, const char *property_name, VAMaterialType material_type)
    {
        if (result == VA_SUCCESS || result == VA_UNCHANGED)
            return;


        VA_ERROR_RESULT(result, "Failed to set ", property_name, " on default material ", VAMaterialTypeToString(material_type));
    }
}

void VADefaultMaterial::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_material_type"), &VADefaultMaterial::get_material_type);
    ClassDB::bind_method(D_METHOD("set_material_type", "value"), &VADefaultMaterial::set_material_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "material_type", PROPERTY_HINT_ENUM, DEFAULT_MATERIAL_NAME_HINT), "set_material_type", "get_material_type");

    ClassDB::bind_method(D_METHOD("get_absorption_lf"), &VADefaultMaterial::get_absorption_lf);
    ClassDB::bind_method(D_METHOD("set_absorption_lf", "value"), &VADefaultMaterial::set_absorption_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "absorption_lf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_absorption_lf", "get_absorption_lf");

    ClassDB::bind_method(D_METHOD("get_absorption_hf"), &VADefaultMaterial::get_absorption_hf);
    ClassDB::bind_method(D_METHOD("set_absorption_hf", "value"), &VADefaultMaterial::set_absorption_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "absorption_hf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_absorption_hf", "get_absorption_hf");

    ClassDB::bind_method(D_METHOD("get_scattering"), &VADefaultMaterial::get_scattering);
    ClassDB::bind_method(D_METHOD("set_scattering", "value"), &VADefaultMaterial::set_scattering);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scattering", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_scattering", "get_scattering");

    ClassDB::bind_method(D_METHOD("get_transmission_lf"), &VADefaultMaterial::get_transmission_lf);
    ClassDB::bind_method(D_METHOD("set_transmission_lf", "value"), &VADefaultMaterial::set_transmission_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_lf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.001,or_greater"), "set_transmission_lf", "get_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_transmission_hf"), &VADefaultMaterial::get_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_transmission_hf", "value"), &VADefaultMaterial::set_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_hf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.001,or_greater"), "set_transmission_hf", "get_transmission_hf");

    ClassDB::bind_method(D_METHOD("get_plane_transmission_lf"), &VADefaultMaterial::get_plane_transmission_lf);
    ClassDB::bind_method(D_METHOD("set_plane_transmission_lf", "value"), &VADefaultMaterial::set_plane_transmission_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plane_transmission_lf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_plane_transmission_lf", "get_plane_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_plane_transmission_hf"), &VADefaultMaterial::get_plane_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_plane_transmission_hf", "value"), &VADefaultMaterial::set_plane_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plane_transmission_hf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_plane_transmission_hf", "get_plane_transmission_hf");

    ClassDB::bind_method(D_METHOD("get_color"), &VADefaultMaterial::get_color);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &VADefaultMaterial::set_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}

VADefaultMaterial::VADefaultMaterial()
{
}

VADefaultMaterial::~VADefaultMaterial()
{
}

void VADefaultMaterial::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    va_godot::VAWorld *va_world = va_godot::find_va_world(this);
    if (!va_world)
    {
        return;
    }

    // Built-in materials already exist in every ::VAWorld, so just override the properties
    ::VAWorld *world = va_world->get_handle();

    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(world, material_type, absorption_lf), "absorption_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(world, material_type, absorption_hf), "absorption_hf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialScattering(world, material_type, scattering), "scattering", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(world, material_type, transmission_lf), "transmission_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(world, material_type, transmission_hf), "transmission_hf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(world, material_type, plane_transmission_lf), "plane_transmission_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(world, material_type, plane_transmission_hf), "plane_transmission_hf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialColor(world, material_type, ToVAudio(color)), "color", material_type);

    va_world_handle = world;
    registered = true;
}

int VADefaultMaterial::get_material_type() const
{
    return material_type;
}

void VADefaultMaterial::set_material_type(int value)
{
    material_type = (VAMaterialType)value;

    reset_properties_to_material_defaults();
}

void VADefaultMaterial::reset_properties_to_material_defaults()
{
    const MaterialDefaults &defaults = get_material_defaults(material_type);

    absorption_lf = defaults.absorption_lf;
    absorption_hf = defaults.absorption_hf;
    scattering = defaults.scattering;
    transmission_lf = defaults.transmission_lf;
    transmission_hf = defaults.transmission_hf;
    plane_transmission_lf = defaults.plane_transmission_lf;
    plane_transmission_hf = defaults.plane_transmission_hf;
    color = defaults.color;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_type, absorption_lf), "absorption_lf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_type, absorption_hf), "absorption_hf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_type, scattering), "scattering", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_type, transmission_lf), "transmission_lf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_type, transmission_hf), "transmission_hf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_type, plane_transmission_lf), "plane_transmission_lf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_type, plane_transmission_hf), "plane_transmission_hf", material_type);
        log_if_material_setter_failed(vaWorldSetMaterialColor(va_world_handle, material_type, ToVAudio(color)), "color", material_type);
    }

    notify_property_list_changed();
}

float VADefaultMaterial::get_absorption_lf() const
{
    return absorption_lf;
}

void VADefaultMaterial::set_absorption_lf(float value)
{
    absorption_lf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_type, value), "absorption_lf", material_type);
    }
}

float VADefaultMaterial::get_absorption_hf() const
{
    return absorption_hf;
}

void VADefaultMaterial::set_absorption_hf(float value)
{
    absorption_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_type, value), "absorption_hf", material_type);
    }
}

float VADefaultMaterial::get_scattering() const
{
    return scattering;
}

void VADefaultMaterial::set_scattering(float value)
{
    scattering = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_type, value), "scattering", material_type);
    }
}

float VADefaultMaterial::get_transmission_lf() const
{
    return transmission_lf;
}

void VADefaultMaterial::set_transmission_lf(float value)
{
    transmission_lf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_type, value), "transmission_lf", material_type);
    }
}

float VADefaultMaterial::get_transmission_hf() const
{
    return transmission_hf;
}

void VADefaultMaterial::set_transmission_hf(float value)
{
    transmission_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_type, value), "transmission_hf", material_type);
    }
}

float VADefaultMaterial::get_plane_transmission_lf() const
{
    return plane_transmission_lf;
}

void VADefaultMaterial::set_plane_transmission_lf(float value)
{
    plane_transmission_lf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_type, value), "plane_transmission_lf", material_type);
    }
}

float VADefaultMaterial::get_plane_transmission_hf() const
{
    return plane_transmission_hf;
}

void VADefaultMaterial::set_plane_transmission_hf(float value)
{
    plane_transmission_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_type, value), "plane_transmission_hf", material_type);
    }
}

Color VADefaultMaterial::get_color() const
{
    return color;
}

void VADefaultMaterial::set_color(const Color &value)
{
    color = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialColor(va_world_handle, material_type, ToVAudio(value)), "color", material_type);
    }
}
