#pragma once

#include <godot_cpp/classes/node.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

// Overrides the acoustic properties of one of the SDK's 23 built-in materials
// (see VAMaterialType in vaudio.h) - picked from a dropdown rather than typed
// as free text/a numeric id. Unlike VACustomMaterial, the target material already
// exists in every ::VAWorld (vaWorldCreate pre-creates ids 0-22), so this
// node only ever calls the vaWorldSetMaterialXxx setters, never
// vaWorldCreateMaterial - and there's no id-collision case to guard against,
// since two VADefaultMaterial nodes picking the same entry is just "last one
// wins" on the same built-in, not an error.
class VADefaultMaterial : public Node
{
    GDCLASS(VADefaultMaterial, Node);

private:
    VAMaterialType material_name = VAMaterialMetal;

    // Matches VAMaterialMetal, the default material_name above.
    float absorption_lf = 0.05f;
    float absorption_hf = 0.02f;
    float scattering = 0.01f;
    float transmission_lf = 0.1f;
    float transmission_hf = 0.05f;
    float plane_transmission_lf = 0.1f;
    float plane_transmission_hf = 0.25f;

    // Set once _enter_tree has pushed the initial values to the SDK - property
    // setters below push live updates to the SDK only once this is true,
    // matching VACustomMaterial's pattern.
    bool registered = false;

    // Cached handle of the owning VAWorld, set in _enter_tree once registered
    // so property setters don't need to re-resolve the VAWorld node every call.
    ::VAWorld *va_world_handle = nullptr;

    // Overwrites the 7 editable properties with the SDK's built-in defaults
    // for material_name, and pushes them to the live world if registered.
    // Called whenever material_name changes so the inspector reflects the
    // newly-selected material instead of stale values from the previous one.
    void reset_properties_to_material_defaults();

protected:
    static void _bind_methods();

public:
    VADefaultMaterial();
    ~VADefaultMaterial();

    void _enter_tree() override;

    int get_material_name() const;
    void set_material_name(int value);

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

    float get_plane_transmission_lf() const;
    void set_plane_transmission_lf(float value);

    float get_plane_transmission_hf() const;
    void set_plane_transmission_hf(float value);
};
