#include "va_custom_material.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_world.h"
#include "va_world_lookup.h"

// Port of vaudio-godot-openal's VACustomMaterial.cs. Only the material-property
// side is ported here - DebugColor/GetDebugColor/_GetConfigurationWarnings/
// _ValidateProperty are editor-only visualisation niceties with no SDK-facing
// behaviour (and vaWorldSetMaterialColor doesn't exist in this C SDK version)
// so they're intentionally left out.

namespace va_godot
{

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
    vaWorldCreateMaterial(world, material_type);
    vaWorldSetMaterialAbsorptionLF(world, material_type, absorption_lf);
    vaWorldSetMaterialAbsorptionHF(world, material_type, absorption_hf);
    vaWorldSetMaterialScattering(world, material_type, scattering);
    vaWorldSetMaterialTransmissionLF(world, material_type, transmission_lf);
    vaWorldSetMaterialTransmissionHF(world, material_type, transmission_hf);
    vaWorldSetMaterialPlaneTransmissionLF(world, material_type, plane_transmission_lf);
    vaWorldSetMaterialPlaneTransmissionHF(world, material_type, plane_transmission_hf);

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
        vaWorldSetMaterialAbsorptionLF(va_world_handle, material_type, value);
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
        vaWorldSetMaterialAbsorptionHF(va_world_handle, material_type, value);
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
        vaWorldSetMaterialScattering(va_world_handle, material_type, value);
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
        vaWorldSetMaterialTransmissionLF(va_world_handle, material_type, value);
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
        vaWorldSetMaterialTransmissionHF(va_world_handle, material_type, value);
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
        vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_type, value);
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
        vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_type, value);
    }
}

} // namespace va_godot
