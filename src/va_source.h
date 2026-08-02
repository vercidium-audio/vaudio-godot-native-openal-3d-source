#pragma once

#include "al_source_node3d.h"

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{
class VAWorld;
class VAEmitter;
}

// Port of vaudio-godot-openal's VASource.cs. Extends ALSourceNode3D (the
// native equivalent of ALSource3D, the C# base class VASource.cs actually
// extends) rather than Node3D directly - see native_godot_plan.md
// "Implement VASource's per-frame result application".
class VASource : public ALSourceNode3D
{
    GDCLASS(VASource, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Private child VAEmitter this VASource owns, matching VASource.cs's
    // CreateEmitter - a source is heard via its own emitter's raytracing
    // results, but never casts its own occlusion/permeation rays (only
    // reverb rays) - see create_emitter's HasRelativeReverb/ray-count
    // overrides.
    va_godot::VAEmitter *emitter = nullptr;

    bool play_when_raytracing_completes = true;
    bool played = false;

    // Exported tuning-knob surface, direct port of vaudio-godot-openal's
    // VASourceProperties.cs - a subset of VAEmitterProperties.cs's groups
    // (no Occlusion/Permeation ray/bounce counts here, since VASource's child
    // emitter always has those forced to 0 in create_emitter - only their
    // EnergyCap fields are exposed, matching the C# reference exactly).
    // Stored here (not just forwarded to `emitter`) so values set before
    // create_emitter() runs (e.g. from a .tscn) survive to be applied once
    // the child emitter exists, same rationale as VAEmitter's own fields.
    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.2f;
    float max_volume = 1.0f;
    int max_echogram_time = 5000;
    int echogram_granularity = 200;
    bool affects_grouped_eax = true;

    float occlusion_energy_cap = 0.15f;
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

    // Pushes every property above onto the child VAEmitter - called once at
    // the end of create_emitter(), after the child's own handle exists.
    void apply_properties_to_emitter();

    void create_emitter();

    void apply_raytracing_results(va_godot::VAEmitter *other);

    // Bound to the child VAEmitter's per-frame result-application hook
    // (VAEmitter doesn't expose an OnRaytracedByAnotherEmitterCallback-style
    // external callback yet - unlike the C# reference - so this native port
    // instead has VASource poll emitter->is_raytraced() itself every frame,
    // matching VASource.cs's own _Process fallback play-trigger path, which
    // exists for exactly this "callback may not have fired yet" case).

protected:
    static void _bind_methods();

public:
    VASource();
    ~VASource();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    bool is_raytraced() const;

    // Matches VASource.cs's Play() override: if raytracing hasn't produced
    // results yet, just arms play_when_raytracing_completes and returns
    // false: actual playback happens once is_raytraced() is true.
    bool play() override;

    bool get_play_when_raytracing_completes() const;
    void set_play_when_raytracing_completes(bool value);

    // Current muffling filter state (direct/dry path only - see
    // ALSourceNode3D::filter's doc comment), read for stats/debug display.
    // 1.0/1.0 until the listener has raytraced this source at least once.
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;

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

    float get_occlusion_energy_cap() const;
    void set_occlusion_energy_cap(float value);
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
};
