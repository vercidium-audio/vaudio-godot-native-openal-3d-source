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

// A spatialised 3D sound that casts its own rays
class VASource : public ALSourceNode3D
{
    GDCLASS(VASource, ALSourceNode3D);

private:
    va_godot::VAWorld *va_world = nullptr;

    va_godot::VAEmitter *emitter = nullptr;

    bool play_when_raytracing_completes = true;
    bool played = false;

    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.15f;
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
    VASource();
    ~VASource();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    bool is_raytraced() const;

    // If raytracing hasn't produced results yet, arms play_when_raytracing_completes and returns false. Actual playback happens once is_raytraced() is true.
    bool play() override;

    bool get_play_when_raytracing_completes() const;
    void set_play_when_raytracing_completes(bool value);

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
