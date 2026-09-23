#include "va_world.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_custom_material.h"
#include "va_engine_util.h"

// Function-local statics (not namespace-scope) so the StringName isn't constructed at static-init time, before Godot's API is bound - doing so crashed the extension's DLL init routine.
const StringName &PrimitiveMetaKey()
{
    static StringName key = "vercidium_audio_primitive";
    return key;
}

const StringName &MaterialMetaKey()
{
    static StringName key = "vercidium_audio_material";
    return key;
}

const StringName &UseFlatTransmissionMetaKey()
{
    static StringName key = "vercidium_audio_use_flat_transmission";
    return key;
}

const StringName &PropagateMetaKey()
{
    static StringName key = "vercidium_audio_propagate";
    return key;
}

namespace va_godot
{

// Names of the built-in VAMaterialType values, indexed by VAMaterialType - the single source of truth for VAWorld::get_material's matching and get_builtin_material_names, which exposes this list to the editor plugin.
static const char *const BUILTIN_MATERIAL_NAMES[] = {
    "air", "brick", "cloth", "concrete", "concretepolished", "dirt",
    "glass", "grass", "gravel", "gyprock", "ice", "leaf", "marble",
    "metal", "mud", "rock", "sand", "snow", "tile", "tree", "water",
    "woodindoor", "woodoutdoor"};

PackedStringArray VAWorld::get_builtin_material_names()
{
    PackedStringArray result;
    result.resize(VAMaterialTypeCount);

    for (int i = 0; i < VAMaterialTypeCount; i++)
        result[i] = BUILTIN_MATERIAL_NAMES[i];

    return result;
}

String VAWorld::get_material_meta_key()
{
    return MaterialMetaKey();
}

String VAWorld::get_use_flat_transmission_meta_key()
{
    return UseFlatTransmissionMetaKey();
}

String VAWorld::get_propagate_meta_key()
{
    return PropagateMetaKey();
}

PropagateMode VAWorld::read_propagate_filter(Node *node, PropagateMode inherited)
{
    if (!node->has_meta(PropagateMetaKey()))
        return inherited;

    String mode = String(node->get_meta(PropagateMetaKey())).to_lower();

    if (mode == "colliders")
        return PropagateMode::Colliders;
    if (mode == "visuals")
        return PropagateMode::Visuals;
    return PropagateMode::All;
}

VAMaterialType VAWorld::get_material(Node *node)
{
    if (!node->has_meta(MaterialMetaKey()))
    {
        return VAMaterialAir;
    }

    String material_string = node->get_meta(MaterialMetaKey());
    String lower = material_string.to_lower();

    // Match custom materials first - custom materials take priority over built-ins.
    for (const auto &kvp : custom_materials)
    {
        if (kvp.second->get_material_name().to_lower() == lower)
        {
            return (VAMaterialType)kvp.first;
        }
    }

    for (int i = 0; i < VAMaterialTypeCount; i++)
    {
        if (lower == String(BUILTIN_MATERIAL_NAMES[i]))
        {
            return (VAMaterialType)i;
        }
    }

    VA_WARN("Unknown material for node ", node->get_name(), ": ", material_string, ". Defaulting to Air");
    return VAMaterialAir;
}

Node *VAWorld::top_level_scene_node(Node *node)
{
    SceneTree *tree = get_tree();
    Node *root = tree ? tree->get_root() : nullptr;

    if (!root)
        return nullptr;

    Node *current = node;

    while (current->get_parent() && current->get_parent() != root)
        current = current->get_parent();

    return current->get_parent() == root ? current : nullptr;
}

void VAWorld::sync_primitive(Node *node)
{
    // Guards being called on a VAWorld whose world hasn't been created yet; should always be non-null in practice since this is only called via the debugger-message capture in register_types.cpp.
    if (!world)
        return;

    // The material that governs the edited node can live on an ancestor. add_primitive only sees that material if the walk starts at or above the node owning it, so restart from the top-level scene node instead of the edited node - otherwise the primitive is removed below and never re-added.
    Node *sync_root = top_level_scene_node(node);
    if (!sync_root)
        sync_root = node;

    // Recursive, matching init_scene's own top-level add_primitive calls - the edited node itself often has no geometry of its own, with the material/transmission override taking effect on descendants. Unlike on_node_added/on_node_removed (non-recursive, since those signals already fire once per node), this is a single one-shot call for the whole edited subtree.
    remove_primitive(sync_root, true);

    // add_primitive re-reads each node's own material/transmission/propagate metadata itself - the defaults here are just the fallback for a node with no metadata at all.
    add_primitive(sync_root, VAMaterialAir, false, PropagateMode::All, true);
}

void VAWorld::validate_materials_in_editor(Node *node)
{
    // get_material already pushes a warning for an unrecognized string - just need to trigger it for every node carrying the metadata.
    if (node->has_meta(MaterialMetaKey()))
    {
        get_material(node);
    }

    TypedArray<Node> children = node->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        validate_materials_in_editor(Object::cast_to<Node>(children[i]));
    }
}

void VAWorld::init_scene()
{
    // Scans from get_tree()->get_root() rather than get_current_scene() - some scenes add their level as a sibling of the current scene rather than a child, so get_current_scene() would miss primitives already baked into it. See va_world_lookup.h's find_va_world for the same fix applied to VAWorld discovery.
    Node *root = get_tree()->get_root();
    if (!root)
    {
        return;
    }

    TypedArray<Node> children = root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        add_primitive(Object::cast_to<Node>(children[i]), VAMaterialAir, false, PropagateMode::All, true);
    }

    get_tree()->connect("node_added", callable_mp(this, &VAWorld::on_node_added));
    get_tree()->connect("node_removed", callable_mp(this, &VAWorld::on_node_removed));
}

// Re-evaluate every node against the current layer mask(s) - invoked when render_layers/collision_layers change at runtime.
void VAWorld::rebuild_primitives()
{
    // Godot runs property setters during scene deserialization, before _ready. The world/tree aren't ready then, so bail early.
    if (!world || !is_inside_tree() || !get_tree())
        return;

    Node *root = get_tree()->get_root();
    if (!root)
        return;

    TypedArray<Node> children = root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        remove_primitive(child, true);
        add_primitive(child, VAMaterialAir, false, PropagateMode::All, true);
    }
}

// This fires for the new parent node AND each of its child nodes separately - parent node is invoked first.
void VAWorld::on_node_added(Node *node)
{
    // Godot's node duplication (e.g. editor copy-paste) copies metadata too, so a duplicate can arrive already carrying its source's PrimitiveMetaKey ref - create_primitive's has_meta guard would then skip it, leaving the duplicate without its own primitive/watcher. Strip any inherited primitive meta first so duplicates always get created fresh.
    if (node->has_meta(PrimitiveMetaKey()))
    {
        node->remove_meta(PrimitiveMetaKey());
    }

    add_primitive(node, VAMaterialAir, false, PropagateMode::All, false);
}

// This fires for the new parent node AND each of its child nodes separately - child nodes are invoked first.
void VAWorld::on_node_removed(Node *node)
{
    remove_primitive(node, false);
}

} // namespace va_godot
