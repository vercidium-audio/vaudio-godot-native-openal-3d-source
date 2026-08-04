#include "va_source_relative.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "va_emitter.h"
#include "va_world.h"

// Finds the (singleton) VAWorld under the current scene root's children -
// same helper as VACustomMaterial/VAEmitter/VASource's find_va_world.
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

void VASourceRelative::_bind_methods()
{
}

VASourceRelative::VASourceRelative()
{
}

VASourceRelative::~VASourceRelative()
{
}

void VASourceRelative::_enter_tree()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    va_world = find_va_world(this);
}

void VASourceRelative::_ready()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    set_relative(true);
}

bool VASourceRelative::play()
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return false;
    }

    if (va_world)
    {
        // Matches VASourceRelative.cs: route into the listener's reverb
        // effect, with no muffling filter (both gains full/unfiltered) -
        // relative sources aren't raytraced themselves and have no emitter
        // of their own, so they always share the listener's room reverb
        // rather than any grouped-EAX slot (nullptr short-circuits
        // get_reverb_effect straight to the listener slot).
        effect = va_world->get_reverb_effect(nullptr);
    }

    update_filter(1.0f, 1.0f);

    return ALSourceNode3D::play();
}
