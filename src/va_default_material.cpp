#include "va_default_material.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "va_world.h"
#include "va_world_lookup.h"

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

            vaWorldWait(world);
            vaWorldDestroy(world);

            loaded = true;
        }

        return defaults[material_name];
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
    vaWorldSetMaterialAbsorptionLF(world, material_name, absorption_lf);
    vaWorldSetMaterialAbsorptionHF(world, material_name, absorption_hf);
    vaWorldSetMaterialScattering(world, material_name, scattering);
    vaWorldSetMaterialTransmissionLF(world, material_name, transmission_lf);
    vaWorldSetMaterialTransmissionHF(world, material_name, transmission_hf);
    vaWorldSetMaterialPlaneTransmissionLF(world, material_name, plane_transmission_lf);
    vaWorldSetMaterialPlaneTransmissionHF(world, material_name, plane_transmission_hf);

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
        vaWorldSetMaterialAbsorptionLF(va_world_handle, material_name, absorption_lf);
        vaWorldSetMaterialAbsorptionHF(va_world_handle, material_name, absorption_hf);
        vaWorldSetMaterialScattering(va_world_handle, material_name, scattering);
        vaWorldSetMaterialTransmissionLF(va_world_handle, material_name, transmission_lf);
        vaWorldSetMaterialTransmissionHF(va_world_handle, material_name, transmission_hf);
        vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_name, plane_transmission_lf);
        vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_name, plane_transmission_hf);
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
        vaWorldSetMaterialAbsorptionLF(va_world_handle, material_name, value);
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
        vaWorldSetMaterialAbsorptionHF(va_world_handle, material_name, value);
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
        vaWorldSetMaterialScattering(va_world_handle, material_name, value);
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
        vaWorldSetMaterialTransmissionLF(va_world_handle, material_name, value);
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
        vaWorldSetMaterialTransmissionHF(va_world_handle, material_name, value);
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
        vaWorldSetMaterialPlaneTransmissionLF(va_world_handle, material_name, value);
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
        vaWorldSetMaterialPlaneTransmissionHF(va_world_handle, material_name, value);
    }
}
