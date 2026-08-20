#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/color.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

// Node that overrides the properties of one of the SDK's 23 built-in materials
class VADefaultMaterial : public Node
{
    GDCLASS(VADefaultMaterial, Node);

private:
    VAMaterialType material_type = VAMaterialMetal;

    float absorption_lf = 0.05f; // Matches VAMaterialMetal, the default material_type above.
    float absorption_hf = 0.02f;
    float scattering = 0.01f;
    float transmission_lf = 0.1f;
    float transmission_hf = 0.05f;
    float flat_transmission_lf = 0.1f;
    float flat_transmission_hf = 0.25f;
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);

    bool registered = false; // true once the material has been configured at runtime by _enter_tree()
    ::VAWorld *va_world_handle = nullptr; // Cached handle of the owning VAWorld

    void reset_properties_to_material_defaults(); // Reverts all properties back to the SDK's built-in defaults

protected:
    static void _bind_methods();

public:
    VADefaultMaterial();
    ~VADefaultMaterial();

    void _enter_tree() override;

    int get_material_type() const;
    void set_material_type(int value);

    float get_absorption_lf() const;
    void set_absorption_lf(float value);

    float get_absorption_hf() const;
    void set_absorption_hf(float value);

    float get_scattering() const;
    void set_scattering(float value);

    float get_transmission_lf() const;
    void set_transmission_lf(float value);

    float get_transmission_hf() const;
    void set_transmission_hf(float value);

    float get_flat_transmission_lf() const;
    void set_flat_transmission_lf(float value);

    float get_flat_transmission_hf() const;
    void set_flat_transmission_hf(float value);

    Color get_color() const;
    void set_color(const Color &value);
};
