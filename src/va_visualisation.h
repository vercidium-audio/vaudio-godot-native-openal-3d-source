#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/color.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

namespace va_godot
{

class VAEmitter;

// Renders vaEmitterSetVisualisationCallback's ray-bounce results as fading diamond sprites.
// Add as a child of any VAEmitter (or VASource/VAListener, both of which are VAEmitters) - this
// node finds that parent, registers itself as the visualisation callback target, and pushes the
// exported ray_count/bounce_count/update_frequency onto the parent's handle. Each callback
// invocation (every update_frequency ms) writes one MultiMesh instance per VAVisualisationData
// entry; per-frame fading is entirely GPU-side (a ShaderMaterial reads each instance's spawn time
// out of its custom data and the fade_in_ms/fade_out_ms/duration_ms/colour uniforms below), so
// nothing here does per-instance work outside the callback itself.
class VAVisualisation : public Node3D
{
    GDCLASS(VAVisualisation, Node3D);

private:
    VAEmitter *emitter = nullptr;

    Ref<MultiMesh> multimesh;
    MultiMeshInstance3D *multimesh_instance = nullptr;
    Ref<ShaderMaterial> shader_material;

    // Ring-buffer write cursor into multimesh's instances - each callback invocation appends
    // ray_count * bounce_count new instances here, wrapping once the buffer is full. The buffer
    // is sized (see required_instance_count) to hold every batch still fading at once, so by the
    // time the cursor wraps back around to overwrite an instance, that instance has already
    // faded out - no bookkeeping needed to free instances early.
    int next_instance = 0;

    int ray_count = 32;
    int bounce_count = 4;
    int update_frequency_ms = 200;

    int fade_in_ms = 750;
    int fade_out_ms = 750;
    int duration_ms = 1500;
    Color color = Color(0.11f, 0.97f, 1.0f, 0.75f);
    float size = 0.1f;
    float normal_offset = 0.05f;
    float max_distance = 30.0f;

    bool waiting_for_world = false;

    void find_emitter();
    void retry_find_emitter(Node *node);
    void create_multimesh();
    void apply_properties_to_emitter();
    void apply_shader_uniforms();

    // Diamonds must stay alive (fading) for duration_ms while new callbacks arrive every
    // update_frequency_ms, so the ring buffer needs room for every batch still fading at once -
    // not just the latest one - or a new callback's writes stomp on still-visible instances from
    // a few callbacks ago. See on_visualisation_data/create_multimesh.
    int required_instance_count() const;

    static Ref<ArrayMesh> build_diamond_mesh();

    static void visualisation_callback_trampoline(::VAEmitter *emitter, VAVisualisationData *data, int count);
    void on_visualisation_data(VAVisualisationData *data, int count);

protected:
    static void _bind_methods();

public:
    VAVisualisation();
    ~VAVisualisation();

    void _enter_tree() override;
    void _exit_tree() override;
    void _ready() override;
    void _process(double delta) override;

    int get_ray_count() const;
    void set_ray_count(int value);
    int get_bounce_count() const;
    void set_bounce_count(int value);
    int get_update_frequency_ms() const;
    void set_update_frequency_ms(int value);

    int get_fade_in_ms() const;
    void set_fade_in_ms(int value);
    int get_fade_out_ms() const;
    void set_fade_out_ms(int value);
    int get_duration_ms() const;
    void set_duration_ms(int value);
    Color get_color() const;
    void set_color(const Color &value);
    float get_size() const;
    void set_size(float value);
    float get_normal_offset() const;
    void set_normal_offset(float value);
    float get_max_distance() const;
    void set_max_distance(float value);
};

} // namespace va_godot
