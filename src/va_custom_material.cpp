#include "va_custom_material.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_conversions.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

// Port of vaudio-godot-openal's VACustomMaterial.cs. Only the material-property
// side is ported here - GetDebugColor/_GetConfigurationWarnings/_ValidateProperty
// are editor-only visualisation niceties with no SDK-facing behaviour, so they're
// intentionally left out.

namespace va_godot
{

namespace
{
    void log_if_material_setter_failed(VAResult result, const char *property_name, const String &material_name)
    {
        if (result == VA_SUCCESS || result == VA_UNCHANGED)
            return;

        VA_ERROR_RESULT(result, "Failed to set ", property_name, " on custom material ", material_name);
    }
}

void VACustomMaterial::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_material_type"), &VACustomMaterial::get_material_type);

    ClassDB::bind_method(D_METHOD("get_material_name"), &VACustomMaterial::get_material_name);
    ClassDB::bind_method(D_METHOD("set_material_name", "value"), &VACustomMaterial::set_material_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "material_name"), "set_material_name", "get_material_name");

    ClassDB::bind_method(D_METHOD("get_absorption_lf"), &VACustomMaterial::get_absorption_lf);
    ClassDB::bind_method(D_METHOD("set_absorption_lf", "value"), &VACustomMaterial::set_absorption_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "absorption_lf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_absorption_lf", "get_absorption_lf");

    ClassDB::bind_method(D_METHOD("get_absorption_hf"), &VACustomMaterial::get_absorption_hf);
    ClassDB::bind_method(D_METHOD("set_absorption_hf", "value"), &VACustomMaterial::set_absorption_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "absorption_hf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_absorption_hf", "get_absorption_hf");

    ClassDB::bind_method(D_METHOD("get_scattering"), &VACustomMaterial::get_scattering);
    ClassDB::bind_method(D_METHOD("set_scattering", "value"), &VACustomMaterial::set_scattering);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scattering", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_scattering", "get_scattering");

    ClassDB::bind_method(D_METHOD("get_transmission_lf"), &VACustomMaterial::get_transmission_lf);
    ClassDB::bind_method(D_METHOD("set_transmission_lf", "value"), &VACustomMaterial::set_transmission_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_lf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.1,or_greater"), "set_transmission_lf", "get_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_transmission_hf"), &VACustomMaterial::get_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_transmission_hf", "value"), &VACustomMaterial::set_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_hf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.1,or_greater"), "set_transmission_hf", "get_transmission_hf");

    ClassDB::bind_method(D_METHOD("get_flat_transmission_lf"), &VACustomMaterial::get_flat_transmission_lf);
    ClassDB::bind_method(D_METHOD("set_flat_transmission_lf", "value"), &VACustomMaterial::set_flat_transmission_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flat_transmission_lf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_flat_transmission_lf", "get_flat_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_flat_transmission_hf"), &VACustomMaterial::get_flat_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_flat_transmission_hf", "value"), &VACustomMaterial::set_flat_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flat_transmission_hf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_flat_transmission_hf", "get_flat_transmission_hf");

    ClassDB::bind_method(D_METHOD("get_color"), &VACustomMaterial::get_color);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &VACustomMaterial::set_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}

VACustomMaterial::VACustomMaterial()
{
}

VACustomMaterial::~VACustomMaterial()
{
}

void VACustomMaterial::_enter_tree()
{
    if (IS_EDITOR_HINT())
        return;

    VAWorld *va_world = find_va_world(this);

    // No VAWorld anywhere in the tree yet - this node's scene may have entered the tree before being
    // parented under the level. Unlike VAEmitter/VASource, materials don't retry via node_added, since
    // they're expected to be defined alongside (and after) the level's VAWorld.
    if (!va_world)
        return;

    // Currently always succeeds - reserved for when material registration can fail (e.g. duplicate names)
    if (!va_world->register_custom_material(this))
        return;

    ::VAWorld *world = va_world->get_handle();

    VAResult create_result = vaWorldCreateMaterial(world, material_type);

    if (create_result != VA_SUCCESS)
    {
        VA_ERROR_RESULT(create_result, "Failed to create material ", material_name);
        return;
    }

    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(world, material_type, absorption_lf), "absorption_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(world, material_type, absorption_hf), "absorption_hf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialScattering(world, material_type, scattering), "scattering", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(world, material_type, transmission_lf), "transmission_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(world, material_type, transmission_hf), "transmission_hf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialFlatTransmissionLF(world, material_type, flat_transmission_lf), "flat_transmission_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialFlatTransmissionHF(world, material_type, flat_transmission_hf), "flat_transmission_hf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialColor(world, material_type, ToVAudio(color)), "color", material_name);

    va_world_handle = world;
    registered = true;
}

int VACustomMaterial::get_material_type() const
{
    return material_type;
}

void VACustomMaterial::set_material_type(int value)
{
    material_type = value;
}

String VACustomMaterial::get_material_name() const
{
    return material_name;
}

void VACustomMaterial::set_material_name(const String &value)
{
    if (registered)
    {
        VA_WARN("Cannot change the name of VACustomMaterial nodes at runtime. Node: ", get_name());
        return;
    }

    material_name = value;
}

float VACustomMaterial::get_absorption_lf() const
{
    return absorption_lf;
}

void VACustomMaterial::set_absorption_lf(float value)
{
    absorption_lf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_type, value), "absorption_lf", material_name);
}

float VACustomMaterial::get_absorption_hf() const
{
    return absorption_hf;
}

void VACustomMaterial::set_absorption_hf(float value)
{
    absorption_hf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_type, value), "absorption_hf", material_name);
}

float VACustomMaterial::get_scattering() const
{
    return scattering;
}

void VACustomMaterial::set_scattering(float value)
{
    scattering = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_type, value), "scattering", material_name);
}

float VACustomMaterial::get_transmission_lf() const
{
    return transmission_lf;
}

void VACustomMaterial::set_transmission_lf(float value)
{
    transmission_lf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_type, value), "transmission_lf", material_name);
}

float VACustomMaterial::get_transmission_hf() const
{
    return transmission_hf;
}

void VACustomMaterial::set_transmission_hf(float value)
{
    transmission_hf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_type, value), "transmission_hf", material_name);
}

float VACustomMaterial::get_flat_transmission_lf() const
{
    return flat_transmission_lf;
}

void VACustomMaterial::set_flat_transmission_lf(float value)
{
    flat_transmission_lf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialFlatTransmissionLF(va_world_handle, material_type, value), "flat_transmission_lf", material_name);
}

float VACustomMaterial::get_flat_transmission_hf() const
{
    return flat_transmission_hf;
}

void VACustomMaterial::set_flat_transmission_hf(float value)
{
    flat_transmission_hf = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialFlatTransmissionHF(va_world_handle, material_type, value), "flat_transmission_hf", material_name);
}

Color VACustomMaterial::get_color() const
{
    return color;
}

void VACustomMaterial::set_color(const Color &value)
{
    color = value;

    if (registered)
        log_if_material_setter_failed(vaWorldSetMaterialColor(va_world_handle, material_type, ToVAudio(value)), "color", material_name);
}

} // namespace va_godot
