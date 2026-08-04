#pragma once

#include <godot_cpp/classes/node3d.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

class ALFilter;
class ALReverbEffect;

namespace va_godot
{

class VAWorld;

// Port of vaudio-godot-openal's VAEmitter.cs, scoped to structural/lifecycle
// behaviour (native_godot_plan.md "Implement VASource's per-frame result
// application"): creation/destruction, listener/target registration, and
// per-frame LPF muffling + reverb-slot resolution. The ~30 exported tuning
// knobs the C# reference has (ReverbRayCount, MaxVolume, TrailColor, Type,
// etc., across Reverb/Muffling/Ambience/Visualisation/Debug/Advanced groups)
// are deliberately NOT ported here - the SDK's own defaults are used - and
// are tracked as a separate future checklist item in native_godot_plan.md.
//
// Name collision note: same as VAWorld/VACustomMaterial, the vaudio C SDK's
// opaque handle type is also called "VAEmitter", in the global namespace.
// This Godot node class lives in namespace va_godot instead - see the
// collision note on VAWorld (va_world.h) for why a namespace was used
// instead of a renamed class. Inside va_godot, the unqualified VAEmitter
// always means this class; ::VAEmitter (global namespace) always means the
// SDK's opaque handle type.
class VAEmitter : public Node3D
{
    GDCLASS(VAEmitter, Node3D);

private:
    VAWorld *va_world = nullptr;
    ::VAEmitter *emitter = nullptr;

    bool is_main_listener = false;

    // Exported tuning-knob surface, direct port of vaudio-godot-openal's
    // VAEmitterProperties.cs. Each setter below mirrors the C#'s
    // "store locally, push to the SDK handle if it already exists" pattern -
    // values set before create_emitter() runs (e.g. from a .tscn) are applied
    // once the handle is created; changes after that push straight through.
    // Debug Rendering colors (TrailColor/ReverbColor/OcclusionColor/
    // PermeationColor/AmbientPermeationColor) are NOT ported - this C SDK
    // version has no vaEmitterSet*Color API to back them (same rationale as
    // VACustomMaterial's DebugColor being skipped).
    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.2f;
    float max_volume = 1.0f;
    int max_echogram_time = 5000;
    int echogram_granularity = 200;
    bool affects_grouped_eax = false;
    bool has_relative_reverb = false;
    float relative_reverb_inner_threshold = 0.6f;
    float relative_reverb_outer_threshold = 0.8f;

    int occlusion_ray_count = 0;
    int occlusion_bounce_count = 0;
    float occlusion_energy_cap = 0.15f;
    int permeation_ray_count = 0;
    int permeation_bounce_count = 0;
    float permeation_energy_cap = 0.15f;

    int ambient_occlusion_ray_count = 0;
    int ambient_occlusion_bounce_count = 0;
    float ambient_occlusion_energy_cap = 0.15f;
    int ambient_permeation_ray_count = 0;
    int ambient_permeation_bounce_count = 0;
    float ambient_permeation_energy_cap = 0.15f;

    int visualisation_ray_count = 0;
    int visualisation_bounce_count = 0;
    int visualisation_update_frequency = 500;

    int type = 0;
    int refresh_ray_count = 16;
    float refresh_distance_threshold = 1.0f;
    int scattering_seed = 0;
    bool clamp_position = true;

    // Pushes every property above onto the (just-created) SDK handle - called
    // once at the end of create_emitter(). Matches VAEmitter.cs's property
    // setters "if (emitter != null) emitter.X = value" firing retroactively
    // once the handle exists.
    void apply_properties_to_handle();

    // Lazily allocated in on_raytraced_by_another_emitter, matching
    // VAEmitter.cs's OnRaytracedByAnotherEmitter - only emitters that are
    // ever raytraced as someone else's target need a muffling filter at all.
    ALFilter *filter = nullptr;

    // Resolved fresh every frame in apply_raytracing_results via
    // VAWorld::get_reverb_effect - not owned by VAEmitter.
    ALReverbEffect *effect = nullptr;

    void create_emitter();
    void remove_emitter();

    void apply_raytracing_results();

    // SDK callback trampolines (registered via vaEmitterSetOnXCallback,
    // identity resolved via vaEmitterSetUserData/vaEmitterGetUserData -
    // mirrors the vaudio-unreal pattern already called out in
    // native_godot_plan.md).
    static void on_raytracing_complete_trampoline(::VAEmitter *emitter);
    static void on_raytraced_by_another_emitter_trampoline(::VAEmitter *source, ::VAEmitter *target);
    static void on_removed_trampoline(::VAEmitter *emitter);

    void on_raytraced_by_another_emitter(::VAEmitter *other);

    // Called once the SDK confirms this emitter has been fully unlinked from
    // the world (vaEmitterSetOnRemovedCallback). Deliberately does NOT call
    // vaEmitterDestroy here - see the warning on
    // vaEmitterSetOnRemovedCallback's declaration in vaudio.h: destroying an
    // emitter from inside its own OnRemoved callback can race
    // VAWorld::~VAWorld's vaWorldWait() drain of an in-flight raytracing
    // cycle that still touches this emitter's SDK-side memory (use-after-
    // free, root-caused while building this method - crashed inside
    // vaWorldWait during engine shutdown). Instead, hands the raw handle to
    // VAWorld::defer_emitter_destroy, which destroys it only after
    // vaWorldWait() has fully returned - see VAWorld::~VAWorld.
    void on_emitter_removed();

protected:
    static void _bind_methods();

public:
    VAEmitter();
    ~VAEmitter();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    bool get_is_main_listener() const;
    void set_is_main_listener(bool value);

    ::VAEmitter *get_handle() const
    {
        return emitter;
    }

    // True once this emitter has produced at least one raytracing result -
    // matches VAEmitter.cs's Raytraced (emitter != null &&
    // !emitter.Initialising).
    bool is_raytraced() const;

    // Thin forward to vaEmitterGetPosition - named distinctly from Node3D's
    // own get_position() (local transform position), which would otherwise
    // silently shadow this in GDScript since Node3D already binds that name.
    Vector3 get_va_position() const;

    // Thin forward to vaEmitterGetWithinWorldBounds - emitters outside the
    // VAWorld's position/size bounds are not raytraced (see vaudio.h).
    bool get_within_world_bounds() const;

    // Thin forwards to the SDK, matching VAEmitter.cs's AddTarget/RemoveTarget
    // and HasRaytracedTarget/GetTargetFilter shortcuts.
    void add_target(VAEmitter *target);
    bool has_raytraced_target(VAEmitter *target) const;
    VALowPassFilter get_target_filter(VAEmitter *target) const;

    ALFilter *get_filter() const
    {
        return filter;
    }

    ALReverbEffect *get_effect() const
    {
        return effect;
    }

    // Exported tuning-knob surface (ADD_PROPERTY'd in _bind_methods) - see
    // the field block above for the matching private storage.
    int get_reverb_ray_count() const;
    void set_reverb_ray_count(int value);
    int get_reverb_bounce_count() const;
    void set_reverb_bounce_count(int value);
    float get_reverb_energy_cap() const;
    void set_reverb_energy_cap(float value);
    float get_max_volume() const;
    void set_max_volume(float value);
    int get_max_echogram_time() const;
    void set_max_echogram_time(int value);
    int get_echogram_granularity() const;
    void set_echogram_granularity(int value);
    bool get_affects_grouped_eax() const;
    void set_affects_grouped_eax(bool value);
    bool get_has_relative_reverb() const;
    void set_has_relative_reverb(bool value);
    float get_relative_reverb_inner_threshold() const;
    void set_relative_reverb_inner_threshold(float value);
    float get_relative_reverb_outer_threshold() const;
    void set_relative_reverb_outer_threshold(float value);

    int get_occlusion_ray_count() const;
    void set_occlusion_ray_count(int value);
    int get_occlusion_bounce_count() const;
    void set_occlusion_bounce_count(int value);
    float get_occlusion_energy_cap() const;
    void set_occlusion_energy_cap(float value);
    int get_permeation_ray_count() const;
    void set_permeation_ray_count(int value);
    int get_permeation_bounce_count() const;
    void set_permeation_bounce_count(int value);
    float get_permeation_energy_cap() const;
    void set_permeation_energy_cap(float value);

    int get_ambient_occlusion_ray_count() const;
    void set_ambient_occlusion_ray_count(int value);
    int get_ambient_occlusion_bounce_count() const;
    void set_ambient_occlusion_bounce_count(int value);
    float get_ambient_occlusion_energy_cap() const;
    void set_ambient_occlusion_energy_cap(float value);
    int get_ambient_permeation_ray_count() const;
    void set_ambient_permeation_ray_count(int value);
    int get_ambient_permeation_bounce_count() const;
    void set_ambient_permeation_bounce_count(int value);
    float get_ambient_permeation_energy_cap() const;
    void set_ambient_permeation_energy_cap(float value);

    int get_visualisation_ray_count() const;
    void set_visualisation_ray_count(int value);
    int get_visualisation_bounce_count() const;
    void set_visualisation_bounce_count(int value);
    int get_visualisation_update_frequency() const;
    void set_visualisation_update_frequency(int value);

    int get_type() const;
    void set_type(int value);
    int get_refresh_ray_count() const;
    void set_refresh_ray_count(int value);
    float get_refresh_distance_threshold() const;
    void set_refresh_distance_threshold(float value);
    int get_scattering_seed() const;
    void set_scattering_seed(int value);
    bool get_clamp_position() const;
    void set_clamp_position(bool value);
};

} // namespace va_godot
