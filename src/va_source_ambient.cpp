#include "va_source_ambient.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_world.h"

// Finds the (singleton) VAWorld under the current scene root's children -
// same helper as VACustomMaterial/VAEmitter/VASource/VASourceRelative's
// find_va_world.
static va_godot::VAWorld *find_va_world(Node *node)
{
    Node *scene_root = node->get_tree()->get_current_scene();
    if (!scene_root)
    {
        return nullptr;
    }

    TypedArray<Node> children = scene_root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        if (va_godot::VAWorld *world = Object::cast_to<va_godot::VAWorld>(children[i]))
        {
            return world;
        }
    }

    return nullptr;
}

void VASourceAmbient::_bind_methods()
{
}

VASourceAmbient::VASourceAmbient()
{
}

VASourceAmbient::~VASourceAmbient()
{
}

void VASourceAmbient::_enter_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    va_world = find_va_world(this);
}

void VASourceAmbient::_ready()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    // Matches VASourceRelative::_ready(): ambient sources aren't raytraced
    // relative to any specific source location (they just replay the
    // listener's ambient muffling gain), so max_distance/reference_distance
    // attenuation would be misleading - force relative/non-spatialised.
    set_relative(true);
}

bool VASourceAmbient::play()
{
    // Matches VASourceAmbient.cs's Play(): don't start until the listener has
    // produced an ambient filter result at least once.
    if (!va_world || !va_world->get_has_ambient_filter())
    {
        return false;
    }

    played = ALSourceNode3D::play();

    // Permanent print (same rationale as VASource's own playback print -
    // cheap and keeps proving the listener-ambient-filter-triggers-playback
    // path works), not a one-off debugging leftover.
    UtilityFunctions::print(
        "[vaudio-godot-native-openal] VASourceAmbient '", get_name(), "' started playback: ",
        played ? "OK" : "FAILED");

    return played;
}

void VASourceAmbient::_process(double delta)
{
    ALSourceNode3D::_process(delta);

    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    if (!va_world || !va_world->get_has_ambient_filter())
    {
        return;
    }

    // Matches VASourceAmbient.cs's `effect = null` (commented-out
    // listenerReverbEffect assignment kept disabled in the reference too) -
    // ambient sources don't send into the room reverb, only the direct path
    // is muffled by the ambient gain.
    effect = nullptr;
    update_filter(va_world->get_ambient_filter_gain_lf(), va_world->get_ambient_filter_gain_hf());

    if (!played)
    {
        played = play();
    }
}
