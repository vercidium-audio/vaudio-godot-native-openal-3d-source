#pragma once

#include <godot_cpp/classes/node.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{

// Port of vaudio-godot-openal's VAMaterial.cs. No transform needed - matched
// by name against the "vercidium_audio_material" metadata string, not by
// scene position - so this is a Node, not a Node3D (matches the C# base
// class, which the original stub here got wrong).
//
// Name collision note: same as VAWorld/VAEmitter, the vaudio C SDK's opaque
// handle type is also called "VAMaterial", in the global namespace. This
// Godot node class lives in namespace va_godot instead - see the collision
// note on VAWorld (va_world.h) for why a namespace was used instead of a
// renamed class. Inside va_godot, the unqualified VAMaterial always means
// this class; ::VAMaterial (global namespace) always means the SDK's opaque
// handle type.
class VAMaterial : public Node
{
    GDCLASS(VAMaterial, Node);

private:
    // Auto-assigned by VAWorld::register_custom_material (lowest unclaimed id
    // >= 1000) - not user-facing, matching vaudio-unreal's
    // UVAudioCustomMaterialAsset::GetMaterialId allocation. Geometry looks
    // materials up by material_name, never by this id (see
    // VAWorld::get_material), so there's nothing for the user to type.
    int material_type = 0;
    String material_name = "CustomMaterial";

    float absorption_lf = 0.02f;
    float absorption_hf = 0.1f;
    float scattering = 0.1f;
    float transmission_lf = 10.0f;
    float transmission_hf = 5.0f;
    float plane_transmission_lf = 0.1f;
    float plane_transmission_hf = 0.25f;

    // Set once the material has been created in the owning VAWorld's ::VAWorld
    // handle (see _enter_tree) - property setters below push live updates to
    // the SDK only once this is true.
    bool registered = false;

    // Cached handle of the owning VAWorld, set in _enter_tree once registered
    // so property setters don't need to re-resolve the VAWorld node every call.
    ::VAWorld *va_world_handle = nullptr;

protected:
    static void _bind_methods();

public:
    VAMaterial();
    ~VAMaterial();

    void _enter_tree() override;

    int get_material_type() const;

    // Called once by VAWorld::register_custom_material to assign the
    // auto-allocated id. Not bound as a Godot property - see material_type's
    // doc comment above.
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

    float get_plane_transmission_lf() const;
    void set_plane_transmission_lf(float value);

    float get_plane_transmission_hf() const;
    void set_plane_transmission_hf(float value);
};

} // namespace va_godot
