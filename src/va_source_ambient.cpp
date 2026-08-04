#include "va_source_ambient.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_world.h"
#include "va_world_lookup.h"

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

    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        // No VAWorld anywhere in the tree yet - stay dormant and retry on
        // every future node addition instead of never recovering - see
        // VAEmitter::_enter_tree's identical pattern for the rationale.
        waiting_for_world = true;
        get_tree()->connect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));
    }
}

void VASourceAmbient::_exit_tree()
{
    if (waiting_for_world)
    {
        if (get_tree() && get_tree()->is_connected("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world)))
        {
            get_tree()->disconnect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));
        }

        waiting_for_world = false;
    }
}

// Re-attempts find_va_world each time a node is added anywhere in the tree -
// see VAEmitter::retry_find_va_world's identical pattern.
void VASourceAmbient::retry_find_va_world(Node *node)
{
    va_world = va_godot::find_va_world(this);

    if (!va_world)
    {
        return;
    }

    get_tree()->disconnect("node_added", callable_mp(this, &VASourceAmbient::retry_find_va_world));
    waiting_for_world = false;
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
    va_godot::VAEmitter *listener = va_world ? va_world->get_listener() : nullptr;
    if (!listener || !listener->is_ambient_filter_ready())
    {
        return false;
    }

    played = ALSourceNode3D::play();

    return played;
}

void VASourceAmbient::_process(double delta)
{
    ALSourceNode3D::_process(delta);

    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    va_godot::VAEmitter *listener = va_world ? va_world->get_listener() : nullptr;
    if (!listener || !listener->is_ambient_filter_ready())
    {
        return;
    }

    // Matches VASourceAmbient.cs's `effect = null` (commented-out
    // listenerReverbEffect assignment kept disabled in the reference too) -
    // ambient sources don't send into the room reverb, only the direct path
    // is muffled by the ambient gain.
    effect = nullptr;
    update_filter(listener->get_ambient_filter_gain_lf(), listener->get_ambient_filter_gain_hf());

    if (!played)
    {
        played = play();
    }
}
