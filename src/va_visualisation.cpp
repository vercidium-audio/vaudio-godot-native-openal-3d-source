#include "va_visualisation.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_conversions.h"
#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_source.h"

namespace va_godot
{

namespace
{
    // GLSL for the diamond sprites; spawn time travels vertex->fragment via a varying since INSTANCE_CUSTOM is only readable in the vertex stage.
    const char *VISUALISATION_SHADER_CODE = R"(
shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never;

uniform float current_time;
uniform float fade_in_ms;
uniform float fade_out_ms;
uniform float duration_ms;
uniform vec4 base_color : source_color;

varying float spawn_time;

void vertex()
{
    spawn_time = INSTANCE_CUSTOM.x;
}

void fragment()
{
    float elapsed_ms = (current_time - spawn_time) * 1000.0;

    if (elapsed_ms < 0.0 || elapsed_ms > duration_ms)
        discard;

    float fade_in = fade_in_ms > 0.0 ? clamp(elapsed_ms / fade_in_ms, 0.0, 1.0) : 1.0;
    float fade_out = fade_out_ms > 0.0 ? clamp((duration_ms - elapsed_ms) / fade_out_ms, 0.0, 1.0) : 1.0;

    ALBEDO = base_color.rgb;
    ALPHA = base_color.a * min(fade_in, fade_out);
}
)";
}

void VAVisualisation::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_ray_count"), &VAVisualisation::get_ray_count);
    ClassDB::bind_method(D_METHOD("set_ray_count", "value"), &VAVisualisation::set_ray_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ray_count"), "set_ray_count", "get_ray_count");

    ClassDB::bind_method(D_METHOD("get_bounce_count"), &VAVisualisation::get_bounce_count);
    ClassDB::bind_method(D_METHOD("set_bounce_count", "value"), &VAVisualisation::set_bounce_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "bounce_count"), "set_bounce_count", "get_bounce_count");

    ClassDB::bind_method(D_METHOD("get_update_frequency_ms"), &VAVisualisation::get_update_frequency_ms);
    ClassDB::bind_method(D_METHOD("set_update_frequency_ms", "value"), &VAVisualisation::set_update_frequency_ms);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "update_frequency_ms"), "set_update_frequency_ms", "get_update_frequency_ms");

    ClassDB::bind_method(D_METHOD("get_fade_in_ms"), &VAVisualisation::get_fade_in_ms);
    ClassDB::bind_method(D_METHOD("set_fade_in_ms", "value"), &VAVisualisation::set_fade_in_ms);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "fade_in_ms"), "set_fade_in_ms", "get_fade_in_ms");

    ClassDB::bind_method(D_METHOD("get_fade_out_ms"), &VAVisualisation::get_fade_out_ms);
    ClassDB::bind_method(D_METHOD("set_fade_out_ms", "value"), &VAVisualisation::set_fade_out_ms);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "fade_out_ms"), "set_fade_out_ms", "get_fade_out_ms");

    ClassDB::bind_method(D_METHOD("get_duration_ms"), &VAVisualisation::get_duration_ms);
    ClassDB::bind_method(D_METHOD("set_duration_ms", "value"), &VAVisualisation::set_duration_ms);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_ms"), "set_duration_ms", "get_duration_ms");

    ClassDB::bind_method(D_METHOD("get_color"), &VAVisualisation::get_color);
    ClassDB::bind_method(D_METHOD("set_color", "value"), &VAVisualisation::set_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");

    ClassDB::bind_method(D_METHOD("get_size"), &VAVisualisation::get_size);
    ClassDB::bind_method(D_METHOD("set_size", "value"), &VAVisualisation::set_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size", PROPERTY_HINT_RANGE, "0.01,5.0,0.01,or_greater"), "set_size", "get_size");

    ClassDB::bind_method(D_METHOD("get_normal_offset"), &VAVisualisation::get_normal_offset);
    ClassDB::bind_method(D_METHOD("set_normal_offset", "value"), &VAVisualisation::set_normal_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_offset", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater"), "set_normal_offset", "get_normal_offset");

    ClassDB::bind_method(D_METHOD("get_max_distance"), &VAVisualisation::get_max_distance);
    ClassDB::bind_method(D_METHOD("set_max_distance", "value"), &VAVisualisation::set_max_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "0.0,500.0,0.1,or_greater"), "set_max_distance", "get_max_distance");
}

VAVisualisation::VAVisualisation()
{
}

VAVisualisation::~VAVisualisation()
{
}

void VAVisualisation::_enter_tree()
{
    if (IS_EDITOR_HINT())
        return;

    find_emitter();

    if (!emitter)
    {
        // Same rationale as VAEmitter::waiting_for_world - this node's scene may enter the tree before being parented under its intended emitter.
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VAVisualisation::retry_find_emitter));
    }
}

void VAVisualisation::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VAVisualisation::retry_find_emitter)))
            get_tree()->disconnect("node_added", callable_mp(this, &VAVisualisation::retry_find_emitter));

        waiting_for_world = false;
    }

    if (emitter && emitter->get_handle())
    {
        vaEmitterSetVisualisationCallback(emitter->get_handle(), nullptr);
        emitter->set_visualisation(nullptr);
    }

    emitter = nullptr;
}

void VAVisualisation::_ready()
{
    if (IS_EDITOR_HINT())
        return;

    create_multimesh();
    set_process(true);
}

void VAVisualisation::_process(double delta)
{
    if (shader_material.is_valid())
        shader_material->set_shader_parameter("current_time", Time::get_singleton()->get_ticks_msec() / 1000.0);
}

void VAVisualisation::find_emitter()
{
    emitter = Object::cast_to<VAEmitter>(get_parent());

    if (!emitter)
    {
        // VASource isn't itself a VAEmitter - it owns a hidden child VAEmitter node instead (see VASource::create_emitter).
        VASource *source = Object::cast_to<VASource>(get_parent());

        if (source)
            emitter = source->get_emitter();
    }

    if (!emitter)
    {
        VA_WARN_NAMED("must be a direct child of a VAEmitter (or VASource/VAListener) to render its visualisation rays.");
        return;
    }

    if (!emitter->get_handle())
    {
        // Parent VAEmitter exists but hasn't created its SDK handle yet - treat as "no emitter yet" and retry via node_added.
        emitter = nullptr;
        return;
    }

    apply_properties_to_emitter();

    emitter->set_visualisation(this);
    vaEmitterSetVisualisationCallback(emitter->get_handle(), &VAVisualisation::visualisation_callback_trampoline);
}

void VAVisualisation::retry_find_emitter(Node *node)
{
    find_emitter();

    if (!emitter)
        return;

    get_tree()->disconnect("node_added", callable_mp(this, &VAVisualisation::retry_find_emitter));
    waiting_for_world = false;
}

void VAVisualisation::apply_properties_to_emitter()
{
    vaEmitterSetVisualisationRayCount(emitter->get_handle(), ray_count);
    vaEmitterSetVisualisationBounceCount(emitter->get_handle(), bounce_count);
    vaEmitterSetVisualisationUpdateFrequency(emitter->get_handle(), update_frequency_ms);
}

int VAVisualisation::required_instance_count() const
{
    int batch_size = MAX(1, ray_count * bounce_count);

    // Batches alive (still fading) at once, +2 as a safety margin against spawn/fade jitter.
    int batches_in_flight = (duration_ms / MAX(1, update_frequency_ms)) + 2;

    return batch_size * batches_in_flight;
}

Ref<ArrayMesh> VAVisualisation::build_diamond_mesh()
{
    // Unit diamond in the local XY plane, facing +Z - orientated per-instance to the ray hit normal (see on_visualisation_data).
    Vector3 top(0, 1, 0);
    Vector3 right(1, 0, 0);
    Vector3 bottom(0, -1, 0);
    Vector3 left(-1, 0, 0);

    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    st->add_vertex(top);
    st->add_vertex(right);
    st->add_vertex(bottom);

    st->add_vertex(top);
    st->add_vertex(bottom);
    st->add_vertex(left);

    return st->commit();
}

void VAVisualisation::create_multimesh()
{
    shader_material.instantiate();

    Ref<Shader> shader;
    shader.instantiate();
    shader->set_code(VISUALISATION_SHADER_CODE);
    shader_material->set_shader(shader);

    apply_shader_uniforms();

    multimesh.instantiate();
    multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
    multimesh->set_use_custom_data(true);
    multimesh->set_mesh(build_diamond_mesh());
    multimesh->set_instance_count(required_instance_count());
    multimesh->set_visible_instance_count(0);

    multimesh_instance = memnew(MultiMeshInstance3D);
    multimesh_instance->set_multimesh(multimesh);
    multimesh_instance->set_material_override(shader_material);

    // VAVisualisationData positions/normals are already in world space - top_level makes this node's own transform
    // (left at identity) the effective global transform, so those world-space positions can be used directly.
    multimesh_instance->set_as_top_level(true);
    multimesh_instance->set_transform(Transform3D());

    // Instance transforms are written from the visualisation callback on a regular _process frame, not the physics
    // step - physics interpolation has nothing valid to interpolate between and just warns on every write.
    multimesh_instance->set_physics_interpolation_mode(Node::PHYSICS_INTERPOLATION_MODE_OFF);

    // Rays can land anywhere within the VAWorld, far outside this node's own transform - a per-instance frustum test would cull them incorrectly.
    multimesh_instance->set_ignore_occlusion_culling(true);
    multimesh_instance->set_extra_cull_margin(1024.0f);

    add_child(multimesh_instance);
}

void VAVisualisation::apply_shader_uniforms()
{
    if (!shader_material.is_valid())
        return;

    shader_material->set_shader_parameter("fade_in_ms", (float)fade_in_ms);
    shader_material->set_shader_parameter("fade_out_ms", (float)fade_out_ms);
    shader_material->set_shader_parameter("duration_ms", (float)duration_ms);
    shader_material->set_shader_parameter("base_color", color);
}

void VAVisualisation::visualisation_callback_trampoline(::VAEmitter *emitter, VAVisualisationData *data, int count)
{
    if (!emitter)
        return;

    VAEmitter *self = static_cast<VAEmitter *>(vaEmitterGetUserData(emitter));

    if (self && self->get_visualisation())
        self->get_visualisation()->on_visualisation_data(data, count);
}

// Called on the main thread from inside VAWorld::_process's vaWorldUpdate() call (see the threading note above
// vaWorldUpdate/vaWorldWait in vaudio.h). Wraps the ring-buffer cursor rather than doing any per-instance cleanup -
// a stale instance simply keeps rendering until overwritten, by which point the shader has faded it to zero alpha.
void VAVisualisation::on_visualisation_data(VAVisualisationData *data, int count)
{
    if (!multimesh.is_valid() || count <= 0)
        return;

    if (!data)
    {
        VA_ERROR_NAMED("Received visualisation data with a null buffer but a non-zero count.");
        return;
    }

    int capacity = multimesh->get_instance_count();
    int needed = MAX(count, required_instance_count());

    if (needed > capacity)
    {
        capacity = needed;
        multimesh->set_instance_count(capacity);
        next_instance = 0;
    }

    double now = Time::get_singleton()->get_ticks_msec() / 1000.0;
    Vector3 up_hint(0, 1, 0);
    Vector3 emitter_position = emitter ? emitter->get_va_position() : Vector3();
    float max_distance_squared = max_distance * max_distance;

    for (int i = 0; i < count; i++)
    {
        Vector3 position = FromVAudio(data[i].position);

        // Skip bounces too far from the emitter rather than writing and hiding them in the shader - keeps ring buffer slots for bounces worth rendering.
        if (max_distance > 0.0f && position.distance_squared_to(emitter_position) > max_distance_squared)
            continue;

        Vector3 normal = FromVAudio(data[i].normal);
        normal = normal.length_squared() > 0.00001f ? normal.normalized() : Vector3(0, 1, 0);

        // Avoid a degenerate basis when the normal is parallel to the up hint.
        Vector3 basis_up = ABS(normal.dot(up_hint)) > 0.999f ? Vector3(1, 0, 0) : up_hint;

        Basis basis = Basis::looking_at(normal, basis_up, true);
        basis.scale(Vector3(size, size, size));

        // Nudge each diamond off the surface it landed on, along the hit normal, to avoid z-fighting.
        Transform3D transform(basis, position + normal * normal_offset);

        multimesh->set_instance_transform(next_instance, transform);
        multimesh->set_instance_custom_data(next_instance, Color(now, 0, 0, 0));

        next_instance = (next_instance + 1) % capacity;
    }

    multimesh->set_visible_instance_count(capacity);
}

int VAVisualisation::get_ray_count() const
{
    return ray_count;
}

void VAVisualisation::set_ray_count(int value)
{
    ray_count = MAX(0, value);

    if (emitter && emitter->get_handle())
        vaEmitterSetVisualisationRayCount(emitter->get_handle(), ray_count);
}

int VAVisualisation::get_bounce_count() const
{
    return bounce_count;
}

void VAVisualisation::set_bounce_count(int value)
{
    bounce_count = MAX(0, value);

    if (emitter && emitter->get_handle())
        vaEmitterSetVisualisationBounceCount(emitter->get_handle(), bounce_count);
}

int VAVisualisation::get_update_frequency_ms() const
{
    return update_frequency_ms;
}

void VAVisualisation::set_update_frequency_ms(int value)
{
    update_frequency_ms = MAX(1, value);

    if (emitter && emitter->get_handle())
        vaEmitterSetVisualisationUpdateFrequency(emitter->get_handle(), update_frequency_ms);
}

int VAVisualisation::get_fade_in_ms() const
{
    return fade_in_ms;
}

void VAVisualisation::set_fade_in_ms(int value)
{
    fade_in_ms = MAX(0, value);
    apply_shader_uniforms();
}

int VAVisualisation::get_fade_out_ms() const
{
    return fade_out_ms;
}

void VAVisualisation::set_fade_out_ms(int value)
{
    fade_out_ms = MAX(0, value);
    apply_shader_uniforms();
}

int VAVisualisation::get_duration_ms() const
{
    return duration_ms;
}

void VAVisualisation::set_duration_ms(int value)
{
    duration_ms = MAX(0, value);
    apply_shader_uniforms();
}

Color VAVisualisation::get_color() const
{
    return color;
}

void VAVisualisation::set_color(const Color &value)
{
    color = value;
    apply_shader_uniforms();
}

float VAVisualisation::get_size() const
{
    return size;
}

void VAVisualisation::set_size(float value)
{
    size = MAX(0.01f, value);
}

float VAVisualisation::get_normal_offset() const
{
    return normal_offset;
}

void VAVisualisation::set_normal_offset(float value)
{
    normal_offset = MAX(0.0f, value);
}

float VAVisualisation::get_max_distance() const
{
    return max_distance;
}

void VAVisualisation::set_max_distance(float value)
{
    max_distance = MAX(0.0f, value);
}

} // namespace va_godot
