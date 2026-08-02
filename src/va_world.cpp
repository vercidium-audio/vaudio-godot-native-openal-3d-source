#include "va_world.h"

#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "al_source_node3d.h"
#include "openal/al_buffer.h"
#include "openal/al_manager.h"
#include "openal/al_source.h"
#include "va_conversions.h"
#include "va_emitter.h"
#include "va_material.h"
#include "va_source.h"
#include "va_source_ambient.h"
#include "va_source_relative.h"

namespace va_godot
{

// Smallest id reserved for custom (non-built-in) materials - matches vaudio.h's
// VAMaterialType comment ("First 1000 values are reserved") and
// vaudio-unreal's FirstCustomMaterialId.
static constexpr int FirstCustomMaterialId = 1000;

void VAWorld::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_position"), &VAWorld::get_position);
    ClassDB::bind_method(D_METHOD("set_position", "value"), &VAWorld::set_position);
    ClassDB::bind_method(D_METHOD("get_size"), &VAWorld::get_size);
    ClassDB::bind_method(D_METHOD("set_size", "value"), &VAWorld::set_size);
    ClassDB::bind_method(D_METHOD("get_epsilon"), &VAWorld::get_epsilon);
    ClassDB::bind_method(D_METHOD("set_epsilon", "value"), &VAWorld::set_epsilon);
    ClassDB::bind_method(D_METHOD("get_world_is_indoors"), &VAWorld::get_world_is_indoors);
    ClassDB::bind_method(D_METHOD("set_world_is_indoors", "value"), &VAWorld::set_world_is_indoors);

    ADD_GROUP("World", "");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "size"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "epsilon"), "set_epsilon", "get_epsilon");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "world_is_indoors"), "set_world_is_indoors", "get_world_is_indoors");

    ClassDB::bind_method(D_METHOD("get_maximum_grouped_eax_count"), &VAWorld::get_maximum_grouped_eax_count);
    ClassDB::bind_method(D_METHOD("set_maximum_grouped_eax_count", "value"), &VAWorld::set_maximum_grouped_eax_count);

    ADD_GROUP("Reverb", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_grouped_eax_count"), "set_maximum_grouped_eax_count", "get_maximum_grouped_eax_count");

    ClassDB::bind_method(D_METHOD("get_meters_per_unit"), &VAWorld::get_meters_per_unit);
    ClassDB::bind_method(D_METHOD("set_meters_per_unit", "value"), &VAWorld::set_meters_per_unit);
    ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &VAWorld::get_speed_of_sound);
    ClassDB::bind_method(D_METHOD("set_speed_of_sound", "value"), &VAWorld::set_speed_of_sound);
    ClassDB::bind_method(D_METHOD("get_humidity"), &VAWorld::get_humidity);
    ClassDB::bind_method(D_METHOD("set_humidity", "value"), &VAWorld::set_humidity);
    ClassDB::bind_method(D_METHOD("get_temperature"), &VAWorld::get_temperature);
    ClassDB::bind_method(D_METHOD("set_temperature", "value"), &VAWorld::set_temperature);
    ClassDB::bind_method(D_METHOD("get_pressure"), &VAWorld::get_pressure);
    ClassDB::bind_method(D_METHOD("set_pressure", "value"), &VAWorld::set_pressure);
    ClassDB::bind_method(D_METHOD("get_reference_frequency_lf"), &VAWorld::get_reference_frequency_lf);
    ClassDB::bind_method(D_METHOD("set_reference_frequency_lf", "value"), &VAWorld::set_reference_frequency_lf);
    ClassDB::bind_method(D_METHOD("get_reference_frequency_hf"), &VAWorld::get_reference_frequency_hf);
    ClassDB::bind_method(D_METHOD("set_reference_frequency_hf", "value"), &VAWorld::set_reference_frequency_hf);

    ADD_GROUP("AirAbsorption", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "meters_per_unit", PROPERTY_HINT_RANGE, "0.0001,1.0,or_greater"), "set_meters_per_unit", "get_meters_per_unit");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound", PROPERTY_HINT_RANGE, "0.0001,1000.0,1,or_greater"), "set_speed_of_sound", "get_speed_of_sound");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "humidity", PROPERTY_HINT_RANGE, "0.0,1.0"), "set_humidity", "get_humidity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature", PROPERTY_HINT_RANGE, "-273.15,100.0,1,or_greater"), "set_temperature", "get_temperature");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pressure", PROPERTY_HINT_RANGE, "0.0,1000000,1,or_greater"), "set_pressure", "get_pressure");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_frequency_lf", PROPERTY_HINT_RANGE, "0.0001,1000,1,or_greater"), "set_reference_frequency_lf", "get_reference_frequency_lf");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_frequency_hf", PROPERTY_HINT_RANGE, "0.0001,20000,1,or_greater"), "set_reference_frequency_hf", "get_reference_frequency_hf");

    ClassDB::bind_method(D_METHOD("get_emitters_outside_the_world_are_muffled"), &VAWorld::get_emitters_outside_the_world_are_muffled);
    ClassDB::bind_method(D_METHOD("set_emitters_outside_the_world_are_muffled", "value"), &VAWorld::set_emitters_outside_the_world_are_muffled);

    ADD_GROUP("Emitters", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emitters_outside_the_world_are_muffled"), "set_emitters_outside_the_world_are_muffled", "get_emitters_outside_the_world_are_muffled");

    ClassDB::bind_method(D_METHOD("get_maximum_concurrency_level"), &VAWorld::get_maximum_concurrency_level);
    ClassDB::bind_method(D_METHOD("set_maximum_concurrency_level", "value"), &VAWorld::set_maximum_concurrency_level);
    ClassDB::bind_method(D_METHOD("get_work_item_count"), &VAWorld::get_work_item_count);
    ClassDB::bind_method(D_METHOD("set_work_item_count", "value"), &VAWorld::set_work_item_count);
    ClassDB::bind_method(D_METHOD("get_pending_shutdown"), &VAWorld::get_pending_shutdown);
    ClassDB::bind_method(D_METHOD("set_pending_shutdown", "value"), &VAWorld::set_pending_shutdown);

    ADD_GROUP("Threading", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_concurrency_level", PROPERTY_HINT_RANGE, "1,32,1,or_greater"), "set_maximum_concurrency_level", "get_maximum_concurrency_level");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "work_item_count", PROPERTY_HINT_RANGE, "1,256,1,or_greater"), "set_work_item_count", "get_work_item_count");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pending_shutdown"), "set_pending_shutdown", "get_pending_shutdown");
    
    ClassDB::bind_method(D_METHOD("get_rendering_enabled"), &VAWorld::get_rendering_enabled);
    ClassDB::bind_method(D_METHOD("set_rendering_enabled", "value"), &VAWorld::set_rendering_enabled);

    ADD_GROUP("Rendering", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rendering_enabled"), "set_rendering_enabled", "get_rendering_enabled");

    // Read-only timing stats (milliseconds) - no ADD_PROPERTY, called
    // directly from GDScript like methods (world.get_main_thread_time()).
    ClassDB::bind_method(D_METHOD("get_main_thread_time"), &VAWorld::get_main_thread_time);
    ClassDB::bind_method(D_METHOD("get_raytracing_time"), &VAWorld::get_raytracing_time);
    ClassDB::bind_method(D_METHOD("get_preparation_time"), &VAWorld::get_preparation_time);
    ClassDB::bind_method(D_METHOD("get_analysis_time"), &VAWorld::get_analysis_time);

    // Read-only GroupedEAX stats, same no-ADD_PROPERTY rationale as the
    // timing stats above.
    ClassDB::bind_method(D_METHOD("get_grouped_eax_count"), &VAWorld::get_grouped_eax_count);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_gain_lf", "index"), &VAWorld::get_grouped_eax_gain_lf);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_gain_hf", "index"), &VAWorld::get_grouped_eax_gain_hf);
    ClassDB::bind_method(D_METHOD("get_grouped_eax_decay_time", "index"), &VAWorld::get_grouped_eax_decay_time);

    // Exports world settings/materials/primitives/emitters to a binary file
    // (vaWorldExport) - callable from GDScript, e.g. wired to a UI button.
    ClassDB::bind_method(D_METHOD("export_to_file", "file_path"), &VAWorld::export_to_file);
}

// Sanity check for architectural decision #7 (native_godot_plan.md): with
// VACoordinateSystemGodot set, a listener facing forward (pitch=0, yaw=0) -
// Godot's -Z - should see a world point directly in front of it (-Z) come
// back as a relative vector still pointing down -Z, with no axis remapping.
static void SanityCheckCoordinateSystem(::VAWorld *world)
{
    VAVector pointInFront = vaVectorCreate(0.0f, 0.0f, -1.0f);
    VAVector relative = vaWorldCalculateListenerRelativePan(world, pointInFront, 0.0f, 0.0f);

    bool asExpected = relative.x > -0.001f && relative.x < 0.001f &&
                       relative.y > -0.001f && relative.y < 0.001f &&
                       relative.z < -0.999f;

    UtilityFunctions::print(
        "VAWorld coordinate system sanity check: point in front (0,0,-1) -> relative (",
        relative.x, ", ", relative.y, ", ", relative.z, ") - ",
        asExpected ? "OK" : "UNEXPECTED");
}

// Sanity check for the OpenAL buffer upload + per-source playback layers
// (native_godot_plan.md): decode project/audio/test.wav through Godot's own
// AudioStreamWAV pipeline, upload it as an OpenAL buffer, then create a
// source, attach the buffer, and play it - proving the decode -> mix_audio ->
// alBufferData -> alGenSources/alSourcePlay round trip works end to end. Not
// conceptually part of VAWorld's own job (raytracing, not audio playback) -
// it's called from VAWorld::_ready()'s already-deferred callback purely
// because that's the first safe point in this codebase where the full engine
// API (AudioServer etc.) is guaranteed ready; AudioServer isn't registered
// yet at GDExtension module-init time (confirmed by a crash when this was
// first tried there). Buffer/source upload has no owning node yet - VASource
// itself is still an empty stub, wiring it up is a separate, not-yet-done
// checklist item - so this has nowhere better to live for now. Kept in place
// permanently once added, same rationale as the coordinate-system check
// above - cheap and keeps proving the decode/playback path works.
static void SanityCheckBufferUpload()
{
    Ref<AudioStreamWAV> stream = AudioStreamWAV::load_from_file("res://audio/test.wav");

    if (stream.is_null())
    {
        UtilityFunctions::push_warning("[vaudio-godot-native-openal] OpenAL buffer sanity check skipped - res://audio/test.wav not found");
        return;
    }

    static ALBuffer buffer;
    bool ok = buffer.load(stream);

    UtilityFunctions::print(
        "[vaudio-godot-native-openal] OpenAL buffer sanity check: ",
        ok ? "OK" : "FAILED",
        " - handle=", (int)buffer.get_handle(),
        " sample_rate=", buffer.get_sample_rate(),
        " duration=", buffer.get_duration_seconds(), "s");

    if (!ok)
    {
        return;
    }

    static ALSource source;
    bool source_ok = source.create();

    if (source_ok)
    {
        source.set_buffer(buffer.get_handle());
        source.set_gain(0.0f); // silent - this is a headless/editor sanity check, not audible playback
        source.set_position(Vector3(0.0f, 0.0f, 0.0f));
        source.play();
    }

    UtilityFunctions::print(
        "[vaudio-godot-native-openal] OpenAL source playback sanity check: ",
        source_ok ? "OK" : "FAILED",
        " - handle=", (int)source.get_handle(),
        " finished=", source.is_finished());
}

// Sanity check for VAEmitter/VASource's per-frame result application
// (native_godot_plan.md): finds the scene's VASource (test_scene.tscn now
// has a "Listener" VAEmitter with is_main_listener=true and a VASource
// sibling), wires the already-uploaded test.wav buffer into it, and plays it
// silently - proving the full chain (VAWorld::register_emitter listener/
// target wiring -> VAEmitter/VASource raytracing callbacks ->
// ALSourceNode3D::play/update_filter -> real ALSource/ALFilter/
// ALReverbEffect calls) round-trips without error. Deliberately not folded
// into SanityCheckBufferUpload above since it depends on the scene tree
// (VASource must already be in the tree), unlike the raw OpenAL-only checks
// there.
static void SanityCheckSourceNode(VAWorld *world)
{
    Node *scene_root = world->get_tree()->get_current_scene();
    if (!scene_root)
    {
        return;
    }

    VASource *va_source = nullptr;
    TypedArray<Node> children = scene_root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        if (VASource *candidate = Object::cast_to<VASource>(children[i]))
        {
            va_source = candidate;
            break;
        }
    }

    if (!va_source)
    {
        return;
    }

    Ref<AudioStreamWAV> stream = AudioStreamWAV::load_from_file("res://audio/test.wav");
    if (stream.is_null())
    {
        return;
    }

    static ALBuffer buffer;
    if (!buffer.load(stream))
    {
        UtilityFunctions::print("[vaudio-godot-native-openal] VASource sanity check: FAILED - buffer upload failed");
        return;
    }

    va_source->set_buffer_handle(buffer.get_handle());
    va_source->set_gain(0.0f); // silent - this is a headless/editor sanity check, not audible playback

    UtilityFunctions::print("[vaudio-godot-native-openal] VASource sanity check: buffer wired, waiting for raytracing to trigger playback");
}

// Sanity check for VASourceRelative/VASourceAmbient (native_godot_plan.md
// "Implement VASourceRelative and VASourceAmbient"): finds each in the scene
// (test_scene.tscn has one of each as a sibling of VAWorld), wires the same
// test.wav buffer in silently, and calls play() once - VASourceRelative
// should succeed immediately (it only needs a VAWorld, not raytracing);
// VASourceAmbient is expected to return false here (no ambient filter yet,
// same "waiting" gate as VASource's raytracing wait) and instead pick itself
// up via its own _process once the listener produces a result - both are
// printed either way so a run's log makes the actual outcome unambiguous.
static void SanityCheckRelativeAndAmbientSources(VAWorld *world)
{
    Node *scene_root = world->get_tree()->get_current_scene();
    if (!scene_root)
    {
        return;
    }

    Ref<AudioStreamWAV> stream = AudioStreamWAV::load_from_file("res://audio/test.wav");
    if (stream.is_null())
    {
        return;
    }

    static ALBuffer buffer;
    if (!buffer.is_valid() && !buffer.load(stream))
    {
        UtilityFunctions::print("[vaudio-godot-native-openal] VASourceRelative/VASourceAmbient sanity check: FAILED - buffer upload failed");
        return;
    }

    TypedArray<Node> children = scene_root->get_children();

    for (int i = 0; i < children.size(); i++)
    {
        if (VASourceRelative *relative_source = Object::cast_to<VASourceRelative>(children[i]))
        {
            relative_source->set_buffer_handle(buffer.get_handle());
            relative_source->set_gain(0.0f); // silent - this is a headless/editor sanity check, not audible playback

            bool ok = relative_source->play();
            UtilityFunctions::print("[vaudio-godot-native-openal] VASourceRelative sanity check: ", ok ? "OK" : "FAILED");
        }
        else if (VASourceAmbient *ambient_source = Object::cast_to<VASourceAmbient>(children[i]))
        {
            ambient_source->set_buffer_handle(buffer.get_handle());
            ambient_source->set_gain(0.0f); // silent - this is a headless/editor sanity check, not audible playback

            UtilityFunctions::print("[vaudio-godot-native-openal] VASourceAmbient sanity check: buffer wired, waiting for listener ambient filter to trigger playback");
        }
    }
}

VAWorld::VAWorld()
{
    // The world should only exist at runtime, not in the editor - matches
    // the is_editor_hint() guard already in _ready()/_exit_tree() below.
    // world stays nullptr; every other method already null-checks it (see
    // ~VAWorld, _process, and the set_* property forwards in va_world.h).
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    world = vaWorldCreate();
    vaWorldSetCoordinateSystem(world, VACoordinateSystemGodot);
    SanityCheckCoordinateSystem(world);

    vaWorldSetUserData(world, this);
    vaWorldSetOnReverbUpdatedCallback(world, &VAWorld::on_reverb_updated_trampoline);

    // Push every VAWorldProperties.cs-ported field's default onto the
    // freshly created handle - Godot only calls each property's setter when
    // the .tscn stores a value different from the class default, so without
    // this the native handle would keep vaWorldCreate()'s own built-in
    // defaults (which don't necessarily match the ones ported from C#)
    // until a scene happens to override one.
    set_position(position);
    set_size(size);
    set_epsilon(epsilon);
    set_world_is_indoors(world_is_indoors);
    set_maximum_grouped_eax_count(maximum_grouped_eax_count);
    set_meters_per_unit(meters_per_unit);
    set_speed_of_sound(speed_of_sound);
    set_humidity(humidity);
    set_temperature(temperature);
    set_pressure(pressure);
    set_reference_frequency_lf(reference_frequency_lf);
    set_reference_frequency_hf(reference_frequency_hf);
    set_emitters_outside_the_world_are_muffled(emitters_outside_the_world_are_muffled);
    set_maximum_concurrency_level(maximum_concurrency_level);
    set_work_item_count(work_item_count);
    set_rendering_enabled(rendering_enabled);

    // ALManager is initialized at GDExtension module-init time (before any
    // node constructor runs - see register_types.cpp), so it's already safe
    // to create OpenAL objects here, unlike Godot engine singletons
    // (AudioServer etc.), which aren't ready until VAWorld::_ready()'s
    // deferred callback (see the module-init-timing gotcha documented on
    // SanityCheckBufferUpload below).
    listener_reverb_effect.create();

    set_process(true);
}

VAWorld::~VAWorld()
{
    if (world)
    {
        // vaWorldWait drains any in-flight raytracing cycle - only safe past
        // this point to destroy emitter handles queued by
        // VAEmitter::on_emitter_removed (see defer_emitter_destroy's doc
        // comment / the use-after-free this avoids).
        vaWorldWait(world);

        for (::VAEmitter *emitter : pending_emitter_destroys)
        {
            vaEmitterDestroy(emitter);
        }
        pending_emitter_destroys.clear();

        vaWorldDestroy(world);
        world = nullptr;
    }
}

void VAWorld::_ready()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    // Wait a frame for the scene to be fully loaded, matching
    // vaudio-godot-openal's CallDeferred(nameof(InitializeScene)).
    callable_mp(this, &VAWorld::init_scene).call_deferred();

    SanityCheckBufferUpload();

    // VASource needs to already be in the tree (init_scene's scan isn't
    // relevant here, but the VASource *node* itself must have run its own
    // _enter_tree first) - deferring one more frame, same pattern as
    // init_scene above, keeps this robust to sibling node ready-order.
    callable_mp_static(&SanityCheckSourceNode).bind(this).call_deferred();

    callable_mp_static(&SanityCheckRelativeAndAmbientSources).bind(this).call_deferred();
}

// VAWorldGodot.cs's _ExitTree port: unsubscribe from the SceneTree signals
// connected in init_scene() and sweep vercidium_audio_primitive/
// vercidium_audio_material metadata off the whole scene, so a VAWorld that's
// removed/reloaded mid-session (scene switch, editor live-reload) doesn't
// leave stale primitive refs or dangling signal connections behind. This was
// deliberately not ported alongside init_scene() originally - see
// native_godot_plan.md's "Add live scene-tree tracking" checklist item.
void VAWorld::_exit_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    if (get_tree())
    {
        if (get_tree()->is_connected("node_added", callable_mp(this, &VAWorld::on_node_added)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VAWorld::on_node_added));
        }

        if (get_tree()->is_connected("node_removed", callable_mp(this, &VAWorld::on_node_removed)))
        {
            get_tree()->disconnect("node_removed", callable_mp(this, &VAWorld::on_node_removed));
        }

        Node *scene_root = get_tree()->get_current_scene();
        if (scene_root)
        {
            remove_primitive(scene_root, true);
        }
    }
}

void VAWorld::_process(double delta)
{
    // Sync the AL listener from the scene's listener VAEmitter before
    // updating the world, matching VAWorldGodot.cs's _Process order (sync AL
    // listener, then world.Update()). No-op until a VAEmitter with
    // is_main_listener=true has entered the tree.
    if (listener)
    {
        ALManager *manager = ALManager::get_singleton();

        if (manager && manager->is_initialized())
        {
            Vector3 position = listener->get_global_position();
            Vector3 forward = -listener->get_global_transform().basis.get_column(2); // Godot's -Z is forward
            Vector3 up = listener->get_global_transform().basis.get_column(1);

            manager->set_listener_position(position);
            manager->set_listener_orientation(forward, up);
        }
    }

    if (world)
    {
        vaWorldUpdate(world);
    }
}

bool VAWorld::register_custom_material(va_godot::VAMaterial *material)
{
    // Auto-allocate the lowest id >= FirstCustomMaterialId not already claimed
    // by another custom material in this world - matches vaudio-unreal's
    // UVAudioCustomMaterialAsset::GetMaterialId, so users never type/manage
    // numeric material ids themselves (see va_material.h's material_type doc
    // comment).
    int type = FirstCustomMaterialId;
    for (const auto &kvp : custom_materials)
    {
        if (kvp.first >= type)
        {
            type = kvp.first + 1;
        }
    }

    material->set_material_type(type);
    custom_materials[type] = material;
    return true;
}

void VAWorld::register_emitter(va_godot::VAEmitter *emitter, bool is_main_listener)
{
    vaWorldAddEmitter(world, emitter->get_handle());

    UtilityFunctions::print(
        "[vaudio-godot-native-openal] VAEmitter added to world. Node: ", emitter->get_name(),
        " is_main_listener=", is_main_listener);

    if (is_main_listener)
    {
        if (!listener)
        {
            listener = emitter;
        }
        else
        {
            UtilityFunctions::push_warning(
                "[vaudio-godot-native-openal] Only one VAEmitter can be the main listener. Node: ", emitter->get_name());
        }

        return;
    }

    if (!listener)
    {
        UtilityFunctions::push_warning(
            "[vaudio-godot-native-openal] VAEmitter nodes cannot be added before the main listener. Node: ", emitter->get_name());
        return;
    }

    listener->add_target(emitter);
}

void VAWorld::on_reverb_updated_trampoline(::VAWorld *world)
{
    VAWorld *self = static_cast<VAWorld *>(vaWorldGetUserData(world));

    if (self)
    {
        self->on_reverb_updated();
    }
}

// VAWorldReverb.cs's CopyReverb port for the shared (non-grouped-EAX)
// fields - every field CopyReverb sets regardless of isGroupedEAX.
static VAEAXReverbParams CopyReverbParams(const VAEAXReverb *eax)
{
    VAEAXReverbParams params;
    params.density = 0.5f; // hardcoded per openal-soft issue #1229 (static when updated live), matching VAWorldReverb.cs's CopyReverb
    params.diffusion = eax->diffusion;
    params.gain = 1.0f; // VAWorldReverb.cs's CopyReverb also hardcodes gain=1 rather than using the SDK's live value
    params.gainHF = eax->gainHF;
    params.gainLF = eax->gainLF;
    params.decayTime = eax->decayTime;
    params.decayHFRatio = eax->decayHFRatio;
    params.decayLFRatio = eax->decayLFRatio;
    params.reflectionsGain = eax->reflectionsGain;
    params.reflectionsDelay = eax->reflectionsDelay;
    params.lateReverbGain = eax->lateReverbGain;
    params.lateReverbDelay = eax->lateReverbDelay;
    params.echoTime = eax->echoTime;
    params.echoDepth = eax->echoDepth;
    params.modulationTime = eax->modulationTime;
    params.modulationDepth = eax->modulationDepth;
    params.airAbsorptionGainHF = eax->airAbsorptionGainHF;
    params.hfReference = eax->hfReference;
    params.lfReference = eax->lfReference;
    params.roomRolloffFactor = eax->roomRolloffFactor;
    params.decayHFLimit = eax->decayHFLimit;

    return params;
}

// VAWorldReverb.cs's OnReverbUpdated/CopyReverb port. Refreshes the single
// global listener slot from the listener's own raytraced EAX, then rebuilds
// every grouped-EAX slot (VAWorldReverb.cs's groupedReverbEffects) from
// vaWorldGetGroupedEAX, blending in the listener-relative pan/gain
// (CopyReverb's "isGroupedEAX" branch) so each grouped zone's reverb fades
// and pans according to the listener's position within it.
void VAWorld::on_reverb_updated()
{
    if (!listener || !listener->get_handle())
    {
        return;
    }

    // VAWorldReverb.cs's OnReverbUpdated ambientFilter update - refreshed
    // alongside the EAX reverb params below since both fire from the same
    // callback.
    VALowPassFilter *ambient_filter = vaEmitterGetAmbientFilter(listener->get_handle());

    if (ambient_filter)
    {
        ambient_filter_gain_lf = ambient_filter->gainLF;
        ambient_filter_gain_hf = ambient_filter->gainHF;
        has_ambient_filter = true;
    }

    VAEAXReverb *eax = vaEmitterGetEAX(listener->get_handle());

    if (eax)
    {
        listener_reverb_effect.set_params(CopyReverbParams(eax));
    }

    int grouped_eax_count = vaWorldGetGroupedEAXCount(world);
    const VAEAXReverb **grouped_eax = vaWorldGetGroupedEAX(world);

    for (int i = 0; i < grouped_eax_count; i++)
    {
        if ((int)grouped_reverb_effects.size() <= i)
        {
            std::unique_ptr<ALReverbEffect> new_effect = std::make_unique<ALReverbEffect>();
            new_effect->create();
            grouped_reverb_effects.push_back(std::move(new_effect));
        }

        VAEAXReverbParams params = CopyReverbParams(grouped_eax[i]);

        // VAWorldReverb.cs's CopyReverb "isGroupedEAX" branch: blend in this
        // slot's gain/direction relative to the listener, so a grouped zone
        // the listener hasn't raytraced yet (or is outside the relative-
        // reverb blend range of) holds its last pan rather than snapping to
        // silence/center - matches vaEAXReverbGetRelativeDirection/
        // vaEAXReverbGetRelativeGain returning NULL for "no entry yet".
        float *relative_gain = vaEAXReverbGetRelativeGain(grouped_eax[i], listener->get_handle());
        VAVector *relative_direction = vaEAXReverbGetRelativeDirection(grouped_eax[i], listener->get_handle());

        if (relative_gain)
        {
            params.effectSlotGain = MIN(1.0f, MAX(0.0f, *relative_gain));
        }

        if (relative_direction)
        {
            // Rotate the raytraced world-space direction into the
            // listener's local space (equivalent to VAWorldReverb.cs's
            // CalculateListenerRelativePan(pan, listener.Pitch, listener.Yaw))
            // so OpenAL's reflections/late-reverb pan vectors - which are
            // listener-relative - point the right way regardless of which
            // way the listener is facing.
            Basis listener_basis = listener->get_global_transform().basis;
            Vector3 pan = listener_basis.xform_inv(FromVAudio(*relative_direction));

            params.reflectionsPan[0] = pan.x;
            params.reflectionsPan[1] = pan.y;
            params.reflectionsPan[2] = pan.z;
            params.lateReverbPan[0] = pan.x;
            params.lateReverbPan[1] = pan.y;
            params.lateReverbPan[2] = pan.z;
        }

        grouped_reverb_effects[i]->set_params(params);
    }
}

ALReverbEffect *VAWorld::get_reverb_effect(::VAEmitter *emitter)
{
    if (emitter && vaEmitterGetAffectsGroupedEAX(emitter))
    {
        int grouped_eax_index = vaEmitterGetGroupedEAXIndex(emitter);

        if (grouped_eax_index >= 0)
        {
            if (grouped_eax_index >= (int)grouped_reverb_effects.size())
            {
                UtilityFunctions::push_warning(
                    "[vaudio-godot-native-openal] Emitter has a grouped EAX index of ", grouped_eax_index,
                    " but only ", (int)grouped_reverb_effects.size(), " EAX presets are available.");
                return &listener_reverb_effect;
            }

            return grouped_reverb_effects[grouped_eax_index].get();
        }
    }

    return &listener_reverb_effect;
}

} // namespace va_godot
