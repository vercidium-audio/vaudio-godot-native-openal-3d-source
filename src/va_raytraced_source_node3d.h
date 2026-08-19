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

// Shared raytracing surface for any spatialised source that casts its own
// reverb/ambience/visualisation rays, regardless of how it gets audio into
// OpenAL (a fixed decoded buffer for VASource, a live pushed/callback buffer
// for VAStreamSource - see godot_stream_plan.md's "give VAStreamSource
// raytracing" follow-up). Factored out of what used to be VASource's own
// private child-VAEmitter machinery so both classes get identical
// reverb/muffling/ambience behaviour instead of duplicating ~25 properties -
// per user: "like an ALSource and an ALStreamSource has all the AL stuff,
// and then either can be wired into a VASource or VAStreamSource... they're
// pretty much the same from the VA perspective, its just that they have a
// different AL base class that does the playback".
//
// Deliberately owns nothing about *when*/*how* playback starts (that's still
// each subclass's own play()/open_stream() override) - only the emitter
// lifecycle, its exported tuning-knob property surface, and per-frame
// application of the listener's raytracing results onto this node's filter/
// reverb slot (inherited from ALSourceNode).
class VARaytracedSourceNode3D : public ALSourceNode3D
{
    GDCLASS(VARaytracedSourceNode3D, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    va_godot::VAEmitter *emitter = nullptr;

    bool raytrace_once = true;

    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.15f;
    float max_volume = 1.0f;
    int max_echogram_time = 5000;
    int echogram_granularity = 100;
    bool affects_grouped_eax = true;
    bool use_listener_reverb = false;

    float occlusion_energy_cap = 0.15f;
    float permeation_energy_cap = 0.15f;

    int ambient_occlusion_ray_count = 0;
    int ambient_occlusion_bounce_count = 0;
    float ambient_occlusion_energy_cap = 0.15f;
    int ambient_permeation_ray_count = 0;
    int ambient_permeation_bounce_count = 0;
    float ambient_permeation_energy_cap = 0.15f;

    int type = 0;
    int refresh_ray_count = 16;
    float refresh_distance_threshold = 1.0f;
    int scattering_seed = 0;

    // Pushes every property above onto the child VAEmitter
    void apply_properties_to_emitter();

    void create_emitter();

    // Set while _enter_tree found no VAWorld yet - see VAEmitter's identical waiting_for_world field for the rationale
    //  - a source's owning scene, e.g. a car, may enter the tree before being parented under the level
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

    void apply_raytracing_results(va_godot::VAEmitter *other);

protected:
    static void _bind_methods();

public:
    VARaytracedSourceNode3D();
    ~VARaytracedSourceNode3D();

    void _enter_tree() override;
    void _exit_tree() override;

    // Not overriding _process here (unlike VAEmitter/VASourceLeech) - each
    // subclass's own _process calls this directly after
    // ALSourceNode3D::_process, since VAStreamSource also needs its own
    // per-frame stream-drain work interleaved (see its _process).
    void process_raytracing(double delta);

    bool is_raytraced() const;

    // The internal VAEmitter this node owns (see create_emitter) - lets
    // VAVisualisation::find_emitter resolve a valid emitter when it's placed
    // as a direct child of this node, since this node itself is not a
    // VAEmitter subclass.
    va_godot::VAEmitter *get_emitter() const
    {
        return emitter;
    }

    bool get_raytrace_once() const;
    void set_raytrace_once(bool value);

    // Returns 0.0f until raytracing completes
    float get_muffling_gain_lf() const;
    float get_muffling_gain_hf() const;

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
    bool get_use_listener_reverb() const;
    void set_use_listener_reverb(bool value);

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

    int get_type() const;
    void set_type(int value);
    int get_refresh_ray_count() const;
    void set_refresh_ray_count(int value);
    float get_refresh_distance_threshold() const;
    void set_refresh_distance_threshold(float value);
    int get_scattering_seed() const;
    void set_scattering_seed(int value);
};
