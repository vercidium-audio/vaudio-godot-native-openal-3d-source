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
    int material_type = 0; // Auto-assigned by VAWorld::register_custom_material, not user-facing
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

    int get_material_type() const;

    void set_material_type(int value);

    String get_material_name() const;
    void set_material_name(const String &value);

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

} // namespace va_godot
