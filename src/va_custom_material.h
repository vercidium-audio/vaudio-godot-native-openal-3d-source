#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/color.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{

class VACustomMaterial : public Node
{
    GDCLASS(VACustomMaterial, Node);

private:
    // Auto-assigned by VAWorld::register_custom_material, not user-facing
    int material_type = 0;
    String material_name = "CustomMaterial";

    float absorption_lf = 0.02f;
    float absorption_hf = 0.1f;
    float scattering = 0.1f;
    float transmission_lf = 10.0f;
    float transmission_hf = 5.0f;
    float flat_transmission_lf = 0.1f;
    float flat_transmission_hf = 0.25f;
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    // true once the material has been created at runtime by _enter_tree()
    bool registered = false;

    // Cached handle of the owning VAWorld
    ::VAWorld *va_world_handle = nullptr;

protected:
    static void _bind_methods();

public:
    VACustomMaterial();
    ~VACustomMaterial();

    void _enter_tree() override;

    // Set the internal ID of the VA material
    void set_material_type(int value);
    // Get the internal ID of the VA material
    int get_material_type() const;

    // Set the name of the custom material
    void set_material_name(const String &value);
    // Get the name of the custom material
    String get_material_name() const;

    // Set the percentage of low-frequency energy that is lost on each bounce
    void set_absorption_lf(float value);
    // Get the percentage of low-frequency energy that is lost on each bounce
    float get_absorption_lf() const;

    // Set the percentage of high-frequency energy that is lost on each bounce
    void set_absorption_hf(float value);
    // Get the percentage of high-frequency energy that is lost on each bounce
    float get_absorption_hf() const;

    // Set the scattering strength when a ray bounces off this material. 0.0 = no scattering, 1.0 skews the ray reflection direction by up to 90 degrees
    void set_scattering(float value);
    // Get the scattering strength when a ray bounces off this material
    float get_scattering() const;

    // Set how many meters a ray must travel through a primitive before it loses all low-frequency energy
    void set_transmission_lf(float value);    
    // Get how many meters a ray must travel through a primitive before it loses all low-frequency energy
    float get_transmission_lf() const;

    // Set how many meters a ray must travel through a primitive before it loses all high-frequency energy
    void set_transmission_hf(float value);
    // Get how many meters a ray must travel through a primitive before it loses all high-frequency energy
    float get_transmission_hf() const;

    // Set the percentage of low-frequency energy that is lost when a permeation ray touches a Plane, Disk, Triangle, Line, non-watertight Mesh, non-enclosed Polygon or open Path primitive, instead of calculating how long the ray spent inside it.
    void set_flat_transmission_lf(float value);
    // Get the percentage of low-frequency energy that is lost when a permeation ray touches a Plane, Disk, Triangle, Line, non-watertight Mesh, non-enclosed Polygon or open Path primitive, instead of calculating how long the ray spent inside it.
    float get_flat_transmission_lf() const;
    
    // Set the percentage of high-frequency energy that is lost when a permeation ray touches a Plane, Disk, Triangle, Line, non-watertight Mesh, non-enclosed Polygon or open Path primitive, instead of calculating how long the ray spent inside it.
    void set_flat_transmission_hf(float value);
    // Get the percentage of high-frequency energy that is lost when a permeation ray touches a Plane, Disk, Triangle, Line, non-watertight Mesh, non-enclosed Polygon or open Path primitive, instead of calculating how long the ray spent inside it.
    float get_flat_transmission_hf() const;
    
    // Set the debug rendering colour for a specific material type (dev build only)
    void set_color(const Color &value);
    // Get the debug rendering colour for a specific material type (dev build only)
    Color get_color() const;

    // Apply the editor properties to the underlying VA material
    void apply_properties_from_editor(float new_absorption_lf, float new_absorption_hf, float new_scattering,
        float new_transmission_lf, float new_transmission_hf, float new_flat_transmission_lf,
        float new_flat_transmission_hf, const Color &new_color);
};

} // namespace va_godot
