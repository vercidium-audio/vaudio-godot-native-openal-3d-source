#include "va_custom_material.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_conversions.h"
#include "va_world.h"
#include "va_world_lookup.h"

// Port of vaudio-godot-openal's VACustomMaterial.cs. Only the material-property
// side is ported here - DebugColor/GetDebugColor/_GetConfigurationWarnings/
// _ValidateProperty are editor-only visualisation niceties with no SDK-facing
// behaviour (and vaWorldSetMaterialColor doesn't exist in this C SDK version)
// so they're intentionally left out.

namespace va_godot
{

namespace
{
    // Shared by every vaWorldSetMaterialXxx call below - they all document the
    // same result vocabulary (VA_INVALID_POINTER/VA_MATERIAL_DOES_NOT_EXIST
    // can't happen here: world is always valid and material_type has just been
    // created via vaWorldCreateMaterial in _enter_tree before any setter runs.
    // VA_INVALID_VALUE/VA_OUT_OF_RANGE are real, since the value comes from
    // the editor's property panel. VA_UNCHANGED is a no-op, not an error).
    void log_if_material_setter_failed(VAResult result, const char *property_name, int material_type)
    {
        if (result == VA_SUCCESS || result == VA_UNCHANGED)
        {
            return;
        }

        UtilityFunctions::push_error(
            "[vaudio-godot-native-openal] VACustomMaterial::set_", property_name,
            " failed for material_type=", material_type, " (VAResult=", VAResultToString(result), ")");
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
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_lf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.001,or_greater"), "set_transmission_lf", "get_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_transmission_hf"), &VACustomMaterial::get_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_transmission_hf", "value"), &VACustomMaterial::set_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transmission_hf", PROPERTY_HINT_RANGE, "0.0001,10.0,0.001,or_greater"), "set_transmission_hf", "get_transmission_hf");

    ClassDB::bind_method(D_METHOD("get_plane_transmission_lf"), &VACustomMaterial::get_plane_transmission_lf);
    ClassDB::bind_method(D_METHOD("set_plane_transmission_lf", "value"), &VACustomMaterial::set_plane_transmission_lf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plane_transmission_lf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_plane_transmission_lf", "get_plane_transmission_lf");

    ClassDB::bind_method(D_METHOD("get_plane_transmission_hf"), &VACustomMaterial::get_plane_transmission_hf);
    ClassDB::bind_method(D_METHOD("set_plane_transmission_hf", "value"), &VACustomMaterial::set_plane_transmission_hf);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "plane_transmission_hf", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_plane_transmission_hf", "get_plane_transmission_hf");
}

VACustomMaterial::VACustomMaterial()
{
}

VACustomMaterial::~VACustomMaterial()
{
}

void VACustomMaterial::_enter_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    VAWorld *va_world = find_va_world(this);
    if (!va_world)
    {
        return;
    }

    if (!va_world->register_custom_material(this))
    {
        return;
    }

    ::VAWorld *world = va_world->get_handle();

    // Unlike the setters below, a failure here means the material was never
    // created, so none of the property setters would have anything to act
    // on - bail instead of pushing them anyway. VA_OUT_OF_RANGE would mean
    // register_custom_material handed out an id < 1000, and VA_ALREADY_EXISTS
    // would mean it handed out an id already claimed - both indicate a bug in
    // that allocation, not user error, so this is logged accordingly.
    VAResult create_result = vaWorldCreateMaterial(world, material_type);
    if (create_result != VA_SUCCESS)
    {
        UtilityFunctions::push_error(
            "[vaudio-godot-native-openal] VACustomMaterial::_enter_tree: vaWorldCreateMaterial failed for material_type=",
            material_type, " (VAResult=", VAResultToString(create_result), ")");
        return;
    }

    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(world, material_type, absorption_lf), "absorption_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(world, material_type, absorption_hf), "absorption_hf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialScattering(world, material_type, scattering), "scattering", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(world, material_type, transmission_lf), "transmission_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(world, material_type, transmission_hf), "transmission_hf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(world, material_type, plane_transmission_lf), "plane_transmission_lf", material_type);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(world, material_type, plane_transmission_hf), "plane_transmission_hf", material_type);

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
        UtilityFunctions::push_warning("[vaudio-godot-native-openal] Cannot change the name of VACustomMaterial nodes at runtime. Node: ", get_name());
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
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_type, value), "absorption_lf", material_type);
    }
}

float VACustomMaterial::get_absorption_hf() const
{
    return absorption_hf;
}

void VACustomMaterial::set_absorption_hf(float value)
{
    absorption_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_type, value), "absorption_hf", material_type);
    }
}

float VACustomMaterial::get_scattering() const
{
    return scattering;
}

void VACustomMaterial::set_scattering(float value)
{
    scattering = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_type, value), "scattering", material_type);
    }
}

float VACustomMaterial::get_transmission_lf() const
{
    return transmission_lf;
}

void VACustomMaterial::set_transmission_lf(float value)
{
    transmission_lf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_type, value), "transmission_lf", material_type);
    }
}

float VACustomMaterial::get_transmission_hf() const
{
    return transmission_hf;
}

void VACustomMaterial::set_transmission_hf(float value)
{
    transmission_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_type, value), "transmission_hf", material_type);
    }
}

float VACustomMaterial::get_plane_transmission_lf() const
{
    return plane_transmission_lf;
}

void VACustomMaterial::set_plane_transmission_lf(float value)
{
    plane_transmission_lf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_type, value), "plane_transmission_lf", material_type);
    }
}

float VACustomMaterial::get_plane_transmission_hf() const
{
    return plane_transmission_hf;
}

void VACustomMaterial::set_plane_transmission_hf(float value)
{
    plane_transmission_hf = value;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_type, value), "plane_transmission_hf", material_type);
    }
}

} // namespace va_godot
