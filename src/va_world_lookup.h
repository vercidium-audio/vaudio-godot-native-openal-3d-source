#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_world.h"

using namespace godot;

namespace va_godot
{

// Find the VAWorld anywhere under the SceneTree root
inline VAWorld *find_va_world_recursive(Node *node)
{
    if (VAWorld *world = Object::cast_to<VAWorld>(node))
        return world;

    TypedArray<Node> children = node->get_children();
    
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);

        if (VAWorld *world = find_va_world_recursive(child))
            return world;
    }

    return nullptr;
}

inline VAWorld *find_va_world(Node *node)
{
    Node *root = node->get_tree()->get_root();

    if (!root)
        return nullptr;

    return find_va_world_recursive(root);
}

} // namespace va_godot
