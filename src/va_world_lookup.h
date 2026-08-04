#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_world.h"

using namespace godot;

namespace va_godot
{

// Finds the (singleton) VAWorld anywhere under the SceneTree root - port of
// NodeExtensions.cs's GetVAWorldParent, widened from a get_current_scene()
// direct-children-only scan to a recursive search from get_root() so a
// VAWorld still resolves when the caller isn't a direct sibling of it, and
// even when the VAWorld's own subtree was added as a sibling of (rather than
// under) the SceneTree's "current scene" - e.g. a level scene instantiated
// and add_child()'d directly onto the tree root from a menu script, as
// truck_town's car_select.gd does via get_parent().add_child(town), never
// becoming get_current_scene() itself. Shared by VACustomMaterial/
// VADefaultMaterial/VAEmitter/VASource/VASourceAmbient/VASourceRelative's
// _enter_tree.
inline VAWorld *find_va_world_recursive(Node *node)
{
    if (VAWorld *world = Object::cast_to<VAWorld>(node))
    {
        return world;
    }

    TypedArray<Node> children = node->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (VAWorld *world = find_va_world_recursive(child))
        {
            return world;
        }
    }

    return nullptr;
}

inline VAWorld *find_va_world(Node *node)
{
    UtilityFunctions::print("[vaudio-godot-native-openal] find_va_world: searching for VAWorld. Caller: ", node->get_name());

    Node *root = node->get_tree()->get_root();
    if (!root)
    {
        UtilityFunctions::print("[vaudio-godot-native-openal] find_va_world: get_root() returned null. Caller: ", node->get_name());
        return nullptr;
    }

    VAWorld *world = find_va_world_recursive(root);

    if (world)
    {
        UtilityFunctions::print("[vaudio-godot-native-openal] find_va_world: found VAWorld '", world->get_name(), "'. Caller: ", node->get_name());
    }
    else
    {
        UtilityFunctions::print("[vaudio-godot-native-openal] find_va_world: no VAWorld found under root '", root->get_name(), "'. Caller: ", node->get_name());
    }

    return world;
}

} // namespace va_godot
