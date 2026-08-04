#include "va_default_material.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "va_conversions.h"
#include "va_world.h"
#include "va_world_lookup.h"

#include <godot_cpp/variant/utility_functions.hpp>

// Dropdown order must match VAMaterialType's declaration order in vaudio.h
// (VAMaterialAir = 0, ...) - the enum property value is used directly as the
// materialId passed to vaWorldSetMaterialXxx below.
static const char *DEFAULT_MATERIAL_NAME_HINT =
    "Air,Brick,Cloth,Concrete,Concrete Polished,Dirt,Glass,Grass,Gravel,"
    "Gyprock,Ice,Leaf,Marble,Metal,Mud,Rock,Sand,Snow,Tile,Tree,Water,"
    "Wood Indoor,Wood Outdoor";

namespace
{
    struct MaterialDefaults
    {
        float absorption_lf;
        float absorption_hf;
        float scattering;
        float transmission_lf;
        float transmission_hf;
        float plane_transmission_lf;
        float plane_transmission_hf;
    };

    // Queried from the SDK on first use and cached for the lifetime of the
    // process, so the vaudio-side defaults never need to be hand-copied here
    // (see material_settings.c in the vaudio SDK repo, which this avoids
    // duplicating). A throwaway ::VAWorld is the only SDK-supported way to
    // read a built-in material's default properties.
    const MaterialDefaults &get_material_defaults(VAMaterialType material_name)
    {
        static MaterialDefaults defaults[VAMaterialTypeCount];
        static bool loaded = false;

        if (!loaded)
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
            }

            // world was just created above and is never shared, so the only
            // documented failure (VA_INVALID_POINTER for a null world) should
            // never trigger - logged defensively in case that assumption ever
            // breaks.
            VAResult wait_result = vaWorldWait(world);
            if (wait_result != VA_SUCCESS)
            {
                UtilityFunctions::push_error(
                    "[vaudio-godot-native-openal] get_material_defaults: vaWorldWait failed on scratch world (VAResult=", VAResultToString(wait_result), ")");
            }

            VAResult destroy_result = vaWorldDestroy(world);
            if (destroy_result != VA_SUCCESS)
            {
                UtilityFunctions::push_error(
                    "[vaudio-godot-native-openal] get_material_defaults: vaWorldDestroy failed on scratch world (VAResult=", VAResultToString(destroy_result), ")");
            }

            loaded = true;
        }

        return defaults[material_name];
    }

    // Shared by every vaWorldSetMaterialXxx call below - they all document the
    // same result vocabulary (VA_INVALID_POINTER/VA_MATERIAL_DOES_NOT_EXIST
    // can't happen here: world is always valid and material_name is one of
    // the 23 built-in ids, which always exist. VA_INVALID_VALUE/
    // VA_OUT_OF_RANGE are real, since the value comes from the editor's
    // property panel or reset_properties_to_material_defaults. VA_UNCHANGED
    // is a no-op, not an error).
    void log_if_material_setter_failed(VAResult result, const char *property_name, VAMaterialType material_name)
    {
        if (result == VA_SUCCESS || result == VA_UNCHANGED)
        {
            return;
        }

        UtilityFunctions::push_error(
            "[vaudio-godot-native-openal] VADefaultMaterial::set_", property_name,
            " failed for material_name=", (int)material_name, " (VAResult=", VAResultToString(result), ")");
    }
}

void VADefaultMaterial::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_material_name"), &VADefaultMaterial::get_material_name);
    ClassDB::bind_method(D_METHOD("set_material_name", "value"), &VADefaultMaterial::set_material_name);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "material_name", PROPERTY_HINT_ENUM, DEFAULT_MATERIAL_NAME_HINT), "set_material_name", "get_material_name");

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
}

VADefaultMaterial::VADefaultMaterial()
{
}

VADefaultMaterial::~VADefaultMaterial()
{
}

void VADefaultMaterial::_enter_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    va_godot::VAWorld *va_world = va_godot::find_va_world(this);
    if (!va_world)
    {
        return;
    }

    // Built-in materials (ids 0-22) already exist in every ::VAWorld - only
    // the properties are overridden here, unlike VACustomMaterial which has to
    // vaWorldCreateMaterial its custom id first.
    ::VAWorld *world = va_world->get_handle();
    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(world, material_name, absorption_lf), "absorption_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(world, material_name, absorption_hf), "absorption_hf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialScattering(world, material_name, scattering), "scattering", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(world, material_name, transmission_lf), "transmission_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(world, material_name, transmission_hf), "transmission_hf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(world, material_name, plane_transmission_lf), "plane_transmission_lf", material_name);
    log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(world, material_name, plane_transmission_hf), "plane_transmission_hf", material_name);

    va_world_handle = world;
    registered = true;
}

int VADefaultMaterial::get_material_name() const
{
    return material_name;
}

void VADefaultMaterial::set_material_name(int value)
{
    material_name = (VAMaterialType)value;

    reset_properties_to_material_defaults();
}

void VADefaultMaterial::reset_properties_to_material_defaults()
{
    const MaterialDefaults &defaults = get_material_defaults(material_name);

    absorption_lf = defaults.absorption_lf;
    absorption_hf = defaults.absorption_hf;
    scattering = defaults.scattering;
    transmission_lf = defaults.transmission_lf;
    transmission_hf = defaults.transmission_hf;
    plane_transmission_lf = defaults.plane_transmission_lf;
    plane_transmission_hf = defaults.plane_transmission_hf;

    if (registered)
    {
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_name, absorption_lf), "absorption_lf", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_name, absorption_hf), "absorption_hf", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_name, scattering), "scattering", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_name, transmission_lf), "transmission_lf", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_name, transmission_hf), "transmission_hf", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_name, plane_transmission_lf), "plane_transmission_lf", material_name);
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_name, plane_transmission_hf), "plane_transmission_hf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionLF(va_world_handle, material_name, value), "absorption_lf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialAbsorptionHF(va_world_handle, material_name, value), "absorption_hf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialScattering(va_world_handle, material_name, value), "scattering", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionLF(va_world_handle, material_name, value), "transmission_lf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialTransmissionHF(va_world_handle, material_name, value), "transmission_hf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_name, value), "plane_transmission_lf", material_name);
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
        log_if_material_setter_failed(vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_name, value), "plane_transmission_hf", material_name);
    }
}
