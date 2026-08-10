#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>

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
class VAVisualisation;

// Port of vaudio-godot-openal's VAEmitter.cs, scoped to structural/lifecycle
// behaviour (native_godot_plan.md "Implement VASource's per-frame result
// application"): creation/destruction, listener/target registration, and
// per-frame LPF muffling + reverb-slot resolution. The exported tuning knobs
// the C# reference has (ReverbRayCount, MaxVolume, TrailColor, Type, etc.,
// across Reverb/Muffling/Ambience/Visualisation/Debug Rendering/Advanced
// groups) are ported below where the C SDK has a backing vaEmitterSet*/Get*
// API; any gaps are tracked as a separate future checklist item in
// native_godot_plan.md.
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
    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.15f;
    float max_volume = 1.0f;
    int max_echogram_time = 5000;
    int echogram_granularity = 100;
    bool affects_grouped_eax = false;
    bool has_relative_reverb = false;
    float relative_reverb_inner_threshold = 0.6f;
    float relative_reverb_outer_threshold = 0.8f;
    bool use_listener_reverb = false;

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

    int type = 0;
    int refresh_ray_count = 16;
    float refresh_distance_threshold = 1.0f;
    int scattering_seed = 0;
    bool clamp_position = true;

    // Top-level property, matches VAEmitter.cs's RaytraceOnce - see
    // on_raytraced_by_another_emitter for the removal behaviour this drives.
    bool raytrace_once = false;

    // Debug rendering colors (vaEmitterSet/Get*Color) - editor-visible only,
    // no effect on raytracing. Defaults match emitter.c's vaEmitterCreate
    // (Color.White/Cyan/Green/Orange/Yellow, each WithAlpha).
    bool random_trail_color = false;
    Color trail_color = Color(1.0f, 1.0f, 1.0f, 25.0f / 255.0f);
    Color reverb_color = Color(27.0f / 255.0f, 247.0f / 255.0f, 255.0f / 255.0f, 51.0f / 255.0f);
    Color occlusion_color = Color(113.0f / 255.0f, 255.0f / 255.0f, 164.0f / 255.0f, 51.0f / 255.0f);
    Color permeation_color = Color(255.0f / 255.0f, 127.0f / 255.0f, 42.0f / 255.0f, 51.0f / 255.0f);
    Color ambient_permeation_color = Color(255.0f / 255.0f, 204.0f / 255.0f, 0.0f / 255.0f, 51.0f / 255.0f);

    // Pushes every property above onto the (just-created) SDK handle - called
    // once at the end of create_emitter(). Matches VAEmitter.cs's property
    // setters "if (emitter != null) emitter.X = value" firing retroactively
    // once the handle exists.
    void apply_properties_to_handle();

    // Lazily allocated in on_raytraced_by_another_emitter, matching
    // VAEmitter.cs's OnRaytracedByAnotherEmitter - only emitters that are
    // ever raytraced as someone else's target need a muffling filter at all.
    ALFilter *filter = nullptr;

    // VAWorldVariables.cs's ambientFilter port, moved from VAWorld onto the
    // listener it actually describes - refreshed from
    // vaEmitterGetAmbientFilter(this emitter's handle) each
    // apply_raytracing_results call (listener only). VASourceAmbient reads
    // this via VAWorld::get_listener() (gain values only - unlike effect,
    // this isn't a real OpenAL object, just the two floats
    // VASourceAmbient.cs's UpdateFilter call needs). ambient_filter_ready
    // stays false until this emitter has raytraced at least once, matching
    // the C#'s "ambientFilter == null" gate in VASourceAmbient.cs.
    float ambient_filter_gain_lf = 1.0f;
    float ambient_filter_gain_hf = 1.0f;
    bool ambient_filter_ready = false;

    // Resolved fresh every frame in apply_raytracing_results via
    // VAWorld::get_reverb_effect - not owned by VAEmitter.
    ALReverbEffect *effect = nullptr;

    // Not owned by VAEmitter - see get_visualisation/set_visualisation.
    VAVisualisation *visualisation = nullptr;

    void create_emitter();
    void remove_emitter();

    // Set while _enter_tree found no VAWorld yet (e.g. this node's scene was
    // instanced/entered the tree before being parented under the level) -
    // listens for get_tree()'s "node_added" signal and retries find_va_world
    // on every node addition, so a car/prop spawned with a pre-configured
    // VAListener/VAEmitter still initialises once it's actually added under
    // a VAWorld-containing scene, instead of erroring out permanently.
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

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
    void _validate_property(PropertyInfo &p_property) const;

    bool get_is_main_listener() const;
    void set_is_main_listener(bool value);

    bool get_raytrace_once() const;
    void set_raytrace_once(bool value);

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

    // Snapshot of this emitter's computed EAX reverb parameters
    // (vaEmitterGetEAX), for runtime debugging from GDScript (e.g. printing
    // a VAListener's reverb state on-screen) - matches CopyReverbParams'
    // field selection in va_world.cpp, plus outsidePercent/returnedPercent
    // since those directly gate whether reverb should be audible at all.
    // Returns an empty Dictionary if this emitter has no handle yet or
    // vaEmitterGetEAX returns null (e.g. before the first raytracing pass).
    // Skips relativeDirections/relativeGains/relativeEmitters - those are
    // keyed per target emitter (vaEAXReverbGetRelativeDirection/Gain), not a
    // single scalar this flat Dictionary shape can represent.
    Dictionary get_eax_debug_info() const;

    // Thin forwards to the SDK, matching VAEmitter.cs's AddTarget/RemoveTarget
    // and HasRaytracedTarget/GetTargetFilter shortcuts.
    void add_target(VAEmitter *target);
    bool has_raytraced_target(VAEmitter *target) const;
    VALowPassFilter get_target_filter(VAEmitter *target) const;

    // VASourceAmbient.cs's "vercidiumAudio?.ambientFilter == null" gate port -
    // false until this emitter has raytraced at least once. Only meaningful
    // on the listener (see apply_raytracing_results).
    bool is_ambient_filter_ready() const
    {
        return ambient_filter_ready;
    }

    float get_ambient_filter_gain_lf() const
    {
        return ambient_filter_gain_lf;
    }

    float get_ambient_filter_gain_hf() const
    {
        return ambient_filter_gain_hf;
    }

    ALFilter *get_filter() const
    {
        return filter;
    }

    ALReverbEffect *get_effect() const
    {
        return effect;
    }

    // Set by VAVisualisation::find_emitter when a VAVisualisation child registers itself against
    // this emitter - lets VAVisualisation::visualisation_callback_trampoline resolve which node
    // to forward vaEmitterSetVisualisationCallback's data to, without stealing
    // vaEmitterSetUserData (already pointed at this VAEmitter by create_emitter(), and relied on
    // by the raytracing-result trampolines above).
    VAVisualisation *get_visualisation() const
    {
        return visualisation;
    }

    void set_visualisation(VAVisualisation *value)
    {
        visualisation = value;
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
    bool get_use_listener_reverb() const;
    void set_use_listener_reverb(bool value);

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

    bool get_random_trail_color() const;
    void set_random_trail_color(bool value);
    Color get_trail_color() const;
    void set_trail_color(const Color &value);
    Color get_reverb_color() const;
    void set_reverb_color(const Color &value);
    Color get_occlusion_color() const;
    void set_occlusion_color(const Color &value);
    Color get_permeation_color() const;
    void set_permeation_color(const Color &value);
    Color get_ambient_permeation_color() const;
    void set_ambient_permeation_color(const Color &value);
};

} // namespace va_godot
