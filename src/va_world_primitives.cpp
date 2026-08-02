#include "va_world.h"

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/world_boundary_shape3d.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_conversions.h"
#include "va_material.h"

// Port of vaudio-godot-openal's VAWorldPrimitives.cs. CSG box/cylinder/sphere,
// CollisionShape3D box/sphere/capsule/cylinder/world-boundary shapes, and
// MeshInstance3D (via ConvertMeshToVAudio in va_conversions.h) are covered.
// CsgPolygon3D/CsgMesh3D and the ConvexPolygon/Concave/HeightMap
// CollisionShape3D variants still need their own mesh-to-triangle-list
// conversion helpers and are left as a no-op fallthrough for now.

// Function-local statics (not namespace-scope) so the StringName isn't
// constructed at static-init time, before Godot's API is bound - doing so
// crashed the extension's DLL init routine.
static const StringName &PrimitiveMetaKey()
{
    static StringName key = "vercidium_audio_primitive";
    return key;
}

static const StringName &MaterialMetaKey()
{
    static StringName key = "vercidium_audio_material";
    return key;
}

namespace va_godot
{

VAMaterialType VAWorld::get_material(Node *node)
{
    if (!node->has_meta(MaterialMetaKey()))
    {
        return VAMaterialAir;
    }

    String material_string = node->get_meta(MaterialMetaKey());
    String lower = material_string.to_lower();

    // Match custom materials first (VAWorldMaterials.cs's GetMaterial does the
    // same - custom materials take priority over built-ins).
    for (const auto &kvp : custom_materials)
    {
        if (kvp.second->get_material_name().to_lower() == lower)
        {
            return (VAMaterialType)kvp.first;
        }
    }

    // Match built-in materials.
    static const char *names[] = {
        "air", "brick", "cloth", "concrete", "concretepolished", "dirt",
        "glass", "grass", "gravel", "gyprock", "ice", "leaf", "marble",
        "metal", "mud", "rock", "sand", "snow", "tile", "tree", "water",
        "woodindoor", "woodoutdoor"};

    for (int i = 0; i < VAMaterialTypeCount; i++)
    {
        if (lower == String(names[i]))
        {
            return (VAMaterialType)i;
        }
    }

    UtilityFunctions::push_warning("[vaudio-godot-native-openal] Unknown material for node ", node->get_name(), ": ", material_string, ". Defaulting to Air");
    return VAMaterialAir;
}

VAPrimitiveRef *VAWorld::attach_watcher(Node3D *node, void *primitive, VAPrimitiveKind kind, std::function<void()> update)
{
    TransformWatcher *watcher = memnew(TransformWatcher);
    watcher->set_on_transform_changed(update);
    node->add_child(watcher);

    VAPrimitiveRef *ref = memnew(VAPrimitiveRef);
    ref->primitive = primitive;
    ref->kind = kind;
    ref->watcher = watcher;
    return ref;
}

void VAWorld::create_primitive(CSGBox3D *csg_box, VAMaterialType material)
{
    if (csg_box->has_meta(PrimitiveMetaKey()))
    {
        return;
    }

    Vector3 scale;
    Transform3D transform = RemoveScale(csg_box->get_global_transform(), scale);

    VAPrismPrimitive *prim = vaPrismPrimitiveCreate();
    vaPrismPrimitiveSetMaterial(prim, material);
    VAVector size = ToVAudio(csg_box->get_size() * scale);
    vaPrismPrimitiveSetSize(prim, size);
    VAMatrix mat = ToVAudio(transform);
    vaPrismPrimitiveSetTransform(prim, &mat);

    vaWorldAddPrimitive_(world, prim);

    VAPrimitiveRef *ref = attach_watcher(csg_box, prim, VAPrimitiveKind::Prism, [this, csg_box, prim]()
    {
        Vector3 updated_scale;
        Transform3D updated_transform = RemoveScale(csg_box->get_global_transform(), updated_scale);
        VAVector updated_size = ToVAudio(csg_box->get_size() * updated_scale);
        vaPrismPrimitiveSetSize(prim, updated_size);
        VAMatrix updated_mat = ToVAudio(updated_transform);
        vaPrismPrimitiveSetTransform(prim, &updated_mat);
    });

    csg_box->set_meta(PrimitiveMetaKey(), ref);
}

void VAWorld::create_primitive(CSGCylinder3D *csg_cylinder, VAMaterialType material)
{
    if (csg_cylinder->has_meta(PrimitiveMetaKey()))
    {
        return;
    }

    if (csg_cylinder->is_cone())
    {
        // Godot's CsgCylinder3D cone is centered at origin (base at -Height/2,
        // apex at +Height/2). VAudio's ConePrimitive has base at Y=0, apex at
        // Y=height - offset the transform down by Height/2 so the base aligns.
        Transform3D offset_transform = csg_cylinder->get_global_transform().translated_local(Vector3(0, -csg_cylinder->get_height() / 2, 0));

        VAConePrimitive *prim = vaConePrimitiveCreate();
        vaConePrimitiveSetMaterial(prim, material);
        vaConePrimitiveSetRadius(prim, csg_cylinder->get_radius());
        vaConePrimitiveSetHeight(prim, csg_cylinder->get_height());
        VAMatrix mat = ToVAudio(offset_transform);
        vaConePrimitiveSetTransform(prim, &mat);

        vaWorldAddPrimitive_(world, prim);

        VAPrimitiveRef *ref = attach_watcher(csg_cylinder, prim, VAPrimitiveKind::Cone, [this, csg_cylinder, prim]()
        {
            Transform3D updated_offset = csg_cylinder->get_global_transform().translated_local(Vector3(0, -csg_cylinder->get_height() / 2, 0));
            vaConePrimitiveSetRadius(prim, csg_cylinder->get_radius());
            vaConePrimitiveSetHeight(prim, csg_cylinder->get_height());
            VAMatrix updated_mat = ToVAudio(updated_offset);
            vaConePrimitiveSetTransform(prim, &updated_mat);
        });

        csg_cylinder->set_meta(PrimitiveMetaKey(), ref);
    }
    else
    {
        VACylinderPrimitive *prim = vaCylinderPrimitiveCreate();
        vaCylinderPrimitiveSetMaterial(prim, material);
        vaCylinderPrimitiveSetRadius(prim, csg_cylinder->get_radius());
        vaCylinderPrimitiveSetLength(prim, csg_cylinder->get_height());
        VAMatrix mat = ToVAudio(csg_cylinder->get_global_transform());
        vaCylinderPrimitiveSetTransform(prim, &mat);

        vaWorldAddPrimitive_(world, prim);

        VAPrimitiveRef *ref = attach_watcher(csg_cylinder, prim, VAPrimitiveKind::Cylinder, [this, csg_cylinder, prim]()
        {
            vaCylinderPrimitiveSetRadius(prim, csg_cylinder->get_radius());
            vaCylinderPrimitiveSetLength(prim, csg_cylinder->get_height());
            VAMatrix updated_mat = ToVAudio(csg_cylinder->get_global_transform());
            vaCylinderPrimitiveSetTransform(prim, &updated_mat);
        });

        csg_cylinder->set_meta(PrimitiveMetaKey(), ref);
    }
}

void VAWorld::create_primitive(CSGSphere3D *csg_sphere, VAMaterialType material)
{
    if (csg_sphere->has_meta(PrimitiveMetaKey()))
    {
        return;
    }

    VASpherePrimitive *prim = vaSpherePrimitiveCreate();
    vaSpherePrimitiveSetMaterial(prim, material);
    vaSpherePrimitiveSetCenter(prim, ToVAudio(csg_sphere->get_global_transform().origin));
    vaSpherePrimitiveSetRadius(prim, csg_sphere->get_radius());

    vaWorldAddPrimitive_(world, prim);

    VAPrimitiveRef *ref = attach_watcher(csg_sphere, prim, VAPrimitiveKind::Sphere, [this, csg_sphere, prim]()
    {
        vaSpherePrimitiveSetCenter(prim, ToVAudio(csg_sphere->get_global_transform().origin));
        vaSpherePrimitiveSetRadius(prim, csg_sphere->get_radius());
    });

    csg_sphere->set_meta(PrimitiveMetaKey(), ref);
}

void VAWorld::create_primitive(CollisionShape3D *collision_shape, VAMaterialType material)
{
    if (collision_shape->has_meta(PrimitiveMetaKey()))
    {
        return;
    }

    Ref<Shape3D> shape = collision_shape->get_shape();
    if (shape.is_null())
    {
        return;
    }

    Transform3D global_transform = collision_shape->get_global_transform();
    Vector3 position = global_transform.origin;
    Vector3 scale = collision_shape->get_scale();

    void *prim = nullptr;
    VAPrimitiveKind kind;

    if (Ref<BoxShape3D> box = shape; box.is_valid())
    {
        Vector3 box_scale;
        Transform3D box_transform = RemoveScale(global_transform, box_scale);

        VAPrismPrimitive *p = vaPrismPrimitiveCreate();
        vaPrismPrimitiveSetMaterial(p, material);
        VAVector size = ToVAudio(box->get_size() * box_scale);
        vaPrismPrimitiveSetSize(p, size);
        VAMatrix mat = ToVAudio(box_transform);
        vaPrismPrimitiveSetTransform(p, &mat);

        vaWorldAddPrimitive_(world, p);
        prim = p;
        kind = VAPrimitiveKind::Prism;
    }
    else if (Ref<SphereShape3D> sphere = shape; sphere.is_valid())
    {
        VASpherePrimitive *p = vaSpherePrimitiveCreate();
        vaSpherePrimitiveSetMaterial(p, material);
        vaSpherePrimitiveSetCenter(p, ToVAudio(position));
        vaSpherePrimitiveSetRadius(p, sphere->get_radius() * scale.x);

        vaWorldAddPrimitive_(world, p);
        prim = p;
        kind = VAPrimitiveKind::Sphere;
    }
    else if (Ref<CapsuleShape3D> capsule = shape; capsule.is_valid())
    {
        // Godot CapsuleShape3D: height is total height including hemispherical
        // caps, radius is the radius. vaudio.CapsulePrimitive: length is the
        // cylinder portion (not including caps). cylinder length = total height - 2 * radius.
        float cylinder_length = capsule->get_height() - 2 * capsule->get_radius();
        if (cylinder_length < 0)
        {
            cylinder_length = 0;
        }

        VACapsulePrimitive *p = vaCapsulePrimitiveCreate();
        vaCapsulePrimitiveSetMaterial(p, material);
        vaCapsulePrimitiveSetRadius(p, capsule->get_radius() * scale.x);
        vaCapsulePrimitiveSetLength(p, cylinder_length * scale.y);
        VAMatrix mat = ToVAudio(global_transform);
        vaCapsulePrimitiveSetTransform(p, &mat);

        vaWorldAddPrimitive_(world, p);
        prim = p;
        kind = VAPrimitiveKind::Capsule;
    }
    else if (Ref<CylinderShape3D> cylinder = shape; cylinder.is_valid())
    {
        VACylinderPrimitive *p = vaCylinderPrimitiveCreate();
        vaCylinderPrimitiveSetMaterial(p, material);
        vaCylinderPrimitiveSetRadius(p, cylinder->get_radius() * scale.x);
        vaCylinderPrimitiveSetLength(p, cylinder->get_height() * scale.y);
        VAMatrix mat = ToVAudio(global_transform);
        vaCylinderPrimitiveSetTransform(p, &mat);

        vaWorldAddPrimitive_(world, p);
        prim = p;
        kind = VAPrimitiveKind::Cylinder;
    }
    else if (Ref<WorldBoundaryShape3D> world_boundary = shape; world_boundary.is_valid())
    {
        // WorldBoundaryShape3D represents an infinite plane - approximate with
        // a large finite plane sized to the raytracing world.
        Plane plane = world_boundary->get_plane();
        Vector3 normal = plane.normal;

        // vaudio.PlanePrimitive lies in the XZ plane at Y=0 in local space,
        // with Y-up as the normal - so basis_y must be the plane normal.
        Vector3 basis_y = normal;
        Vector3 basis_x = basis_y.cross(Vector3(0, 0, -1)).normalized();

        if (basis_x.length_squared() < 0.001f)
        {
            basis_x = basis_y.cross(Vector3(1, 0, 0)).normalized();
        }

        Vector3 basis_z = basis_x.cross(basis_y).normalized();

        // The plane position is: point on plane (normal * D) + the collision
        // shape's global position.
        Vector3 plane_position = normal * plane.d + global_transform.origin;

        Basis plane_basis;
        plane_basis.set_column(0, basis_x);
        plane_basis.set_column(1, basis_y);
        plane_basis.set_column(2, basis_z);

        Transform3D plane_transform(plane_basis, plane_position);

        VAVector world_size = vaWorldGetSize(world);
        float world_magnitude = Vector3(world_size.x, world_size.y, world_size.z).length();

        VAPlanePrimitive *p = vaPlanePrimitiveCreate();
        vaPlanePrimitiveSetMaterial(p, material);
        // Use the max world size to ensure the plane covers the raytracing
        // scene, *2 in case the plane is positioned in the corner of the world.
        vaPlanePrimitiveSetWidth(p, world_magnitude * 2);
        vaPlanePrimitiveSetHeight(p, world_magnitude * 2);
        VAMatrix mat = ToVAudio(plane_transform);
        vaPlanePrimitiveSetTransform(p, &mat);

        vaWorldAddPrimitive_(world, p);
        prim = p;
        kind = VAPrimitiveKind::Plane;
    }
    else
    {
        // ConvexPolygonShape3D / ConcavePolygonShape3D / HeightMapShape3D all
        // need mesh-to-triangle-list conversion - separate checklist item.
        return;
    }

    VAPrimitiveRef *ref = memnew(VAPrimitiveRef);
    ref->primitive = prim;
    ref->kind = kind;

    TransformWatcher *watcher = memnew(TransformWatcher);
    watcher->set_on_transform_changed([this, collision_shape, ref]()
    {
        update_collision_shape_primitive(collision_shape, ref);
    });
    collision_shape->add_child(watcher);
    ref->watcher = watcher;

    collision_shape->set_meta(PrimitiveMetaKey(), ref);
}

void VAWorld::create_primitive(MeshInstance3D *mesh_instance, VAMaterialType material)
{
    if (mesh_instance->has_meta(PrimitiveMetaKey()))
    {
        return;
    }

    Ref<Mesh> mesh = mesh_instance->get_mesh();
    if (mesh.is_null())
    {
        UtilityFunctions::push_warning("[vaudio-godot-native-openal] MeshInstance3D ", mesh_instance->get_name(), " will not affect raytracing as it has no mesh");
        return;
    }

    VAVector min, max;
    std::vector<VAVector> triangles = ConvertMeshToVAudio(mesh, min, max);
    if (triangles.empty())
    {
        return;
    }

    VAMatrix mat = ToVAudio(mesh_instance->get_global_transform());

    VAMeshPrimitive *prim = nullptr;
    vaMeshPrimitiveCreate(material, triangles.data(), (int)triangles.size(), min, max, &mat, &prim);
    // TODO - make this a metadata / inspector flag in Godot.
    vaMeshPrimitiveSetSupports3DPermeation(prim, true);

    vaWorldAddPrimitive_(world, prim);

    VAPrimitiveRef *ref = attach_watcher(mesh_instance, prim, VAPrimitiveKind::Mesh, [this, mesh_instance, prim]()
    {
        VAMatrix updated_mat = ToVAudio(mesh_instance->get_global_transform());
        vaMeshPrimitiveSetTransform(prim, &updated_mat);
    });

    mesh_instance->set_meta(PrimitiveMetaKey(), ref);
}

void VAWorld::update_collision_shape_primitive(CollisionShape3D *collision_shape, VAPrimitiveRef *ref)
{
    Transform3D global_transform = collision_shape->get_global_transform();
    Vector3 scale = collision_shape->get_scale();
    Ref<Shape3D> shape = collision_shape->get_shape();

    switch (ref->kind)
    {
        case VAPrimitiveKind::Sphere:
        {
            VASpherePrimitive *p = (VASpherePrimitive *)ref->primitive;
            Ref<SphereShape3D> sphere = shape;
            vaSpherePrimitiveSetCenter(p, ToVAudio(global_transform.origin));
            vaSpherePrimitiveSetRadius(p, sphere->get_radius() * scale.x);
            break;
        }
        case VAPrimitiveKind::Prism:
        {
            VAPrismPrimitive *p = (VAPrismPrimitive *)ref->primitive;
            Ref<BoxShape3D> box = shape;
            Vector3 box_scale;
            Transform3D box_transform = RemoveScale(global_transform, box_scale);
            VAVector size = ToVAudio(box->get_size() * box_scale);
            vaPrismPrimitiveSetSize(p, size);
            VAMatrix mat = ToVAudio(box_transform);
            vaPrismPrimitiveSetTransform(p, &mat);
            break;
        }
        case VAPrimitiveKind::Capsule:
        {
            VACapsulePrimitive *p = (VACapsulePrimitive *)ref->primitive;
            Ref<CapsuleShape3D> capsule = shape;
            float cylinder_length = capsule->get_height() - 2 * capsule->get_radius();
            if (cylinder_length < 0)
            {
                cylinder_length = 0;
            }
            vaCapsulePrimitiveSetRadius(p, capsule->get_radius() * scale.x);
            vaCapsulePrimitiveSetLength(p, cylinder_length * scale.y);
            VAMatrix mat = ToVAudio(global_transform);
            vaCapsulePrimitiveSetTransform(p, &mat);
            break;
        }
        case VAPrimitiveKind::Cylinder:
        {
            VACylinderPrimitive *p = (VACylinderPrimitive *)ref->primitive;
            Ref<CylinderShape3D> cylinder = shape;
            vaCylinderPrimitiveSetRadius(p, cylinder->get_radius() * scale.x);
            vaCylinderPrimitiveSetLength(p, cylinder->get_height() * scale.y);
            VAMatrix mat = ToVAudio(global_transform);
            vaCylinderPrimitiveSetTransform(p, &mat);
            break;
        }
        case VAPrimitiveKind::Plane:
        {
            VAPlanePrimitive *p = (VAPlanePrimitive *)ref->primitive;
            Ref<WorldBoundaryShape3D> world_boundary = shape;
            Plane plane = world_boundary->get_plane();
            Vector3 normal = plane.normal;

            Vector3 basis_y = normal;
            Vector3 basis_x = basis_y.cross(Vector3(0, 0, -1)).normalized();
            if (basis_x.length_squared() < 0.001f)
            {
                basis_x = basis_y.cross(Vector3(1, 0, 0)).normalized();
            }
            Vector3 basis_z = basis_x.cross(basis_y).normalized();

            Vector3 plane_position = normal * plane.d + global_transform.origin;

            Basis plane_basis;
            plane_basis.set_column(0, basis_x);
            plane_basis.set_column(1, basis_y);
            plane_basis.set_column(2, basis_z);

            Transform3D plane_transform(plane_basis, plane_position);
            VAMatrix mat = ToVAudio(plane_transform);
            vaPlanePrimitiveSetTransform(p, &mat);
            break;
        }
        default:
            break;
    }
}

void VAWorld::add_primitive(Node *node, VAMaterialType material, bool recursive)
{
    // Use this specific material rather than the parent material.
    if (node->has_meta(MaterialMetaKey()))
    {
        material = get_material(node);
    }

    // Ignore nodes without materials.
    if (material != VAMaterialAir)
    {
        if (CSGBox3D *csg_box = Object::cast_to<CSGBox3D>(node))
        {
            create_primitive(csg_box, material);
            UtilityFunctions::print("VAWorld: added CSGBox3D primitive for node ", node->get_name());
        }
        else if (CSGCylinder3D *csg_cylinder = Object::cast_to<CSGCylinder3D>(node))
        {
            create_primitive(csg_cylinder, material);
            UtilityFunctions::print("VAWorld: added CSGCylinder3D primitive for node ", node->get_name());
        }
        else if (CSGSphere3D *csg_sphere = Object::cast_to<CSGSphere3D>(node))
        {
            create_primitive(csg_sphere, material);
            UtilityFunctions::print("VAWorld: added CSGSphere3D primitive for node ", node->get_name());
        }
        else if (CollisionShape3D *collision_shape = Object::cast_to<CollisionShape3D>(node))
        {
            create_primitive(collision_shape, material);
            UtilityFunctions::print("VAWorld: added CollisionShape3D primitive for node ", node->get_name());
        }
        else if (MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(node))
        {
            create_primitive(mesh_instance, material);
            UtilityFunctions::print("VAWorld: added MeshInstance3D primitive for node ", node->get_name());
        }
    }

    if (recursive)
    {
        TypedArray<Node> children = node->get_children();
        for (int i = 0; i < children.size(); i++)
        {
            add_primitive(Object::cast_to<Node>(children[i]), material, true);
        }
    }
}

void VAWorld::remove_primitive(Node *node, bool recursive)
{
    // When a node is removed from the scene, remove it from the raytracing
    // simulation too.
    if (node->has_meta(PrimitiveMetaKey()))
    {
        Ref<VAPrimitiveRef> ref = node->get_meta(PrimitiveMetaKey());

        if (ref.is_valid())
        {
            if (ref->watcher)
            {
                ref->watcher->queue_free();
            }

            switch (ref->kind)
            {
                case VAPrimitiveKind::Prism:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaPrismPrimitiveDestroy((VAPrismPrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Cylinder:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaCylinderPrimitiveDestroy((VACylinderPrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Cone:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaConePrimitiveDestroy((VAConePrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Sphere:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaSpherePrimitiveDestroy((VASpherePrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Capsule:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaCapsulePrimitiveDestroy((VACapsulePrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Plane:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaPlanePrimitiveDestroy((VAPlanePrimitive *)ref->primitive);
                    break;
                case VAPrimitiveKind::Mesh:
                    vaWorldRemovePrimitive_(world, ref->primitive);
                    vaMeshPrimitiveDestroy((VAMeshPrimitive *)ref->primitive);
                    break;
            }
        }

        node->remove_meta(PrimitiveMetaKey());
    }

    if (recursive)
    {
        TypedArray<Node> children = node->get_children();
        for (int i = 0; i < children.size(); i++)
        {
            remove_primitive(Object::cast_to<Node>(children[i]), true);
        }
    }
}

void VAWorld::init_scene()
{
    Node *scene_root = get_tree()->get_current_scene();
    if (!scene_root)
    {
        return;
    }

    TypedArray<Node> children = scene_root->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        add_primitive(Object::cast_to<Node>(children[i]), VAMaterialAir, true);
    }

    get_tree()->connect("node_added", callable_mp(this, &VAWorld::on_node_added));
    get_tree()->connect("node_removed", callable_mp(this, &VAWorld::on_node_removed));
}

// This fires for the new parent node AND each of its child nodes separately -
// parent node is invoked first.
void VAWorld::on_node_added(Node *node)
{
    add_primitive(node, VAMaterialAir, false);
}

// This fires for the new parent node AND each of its child nodes separately -
// child nodes are invoked first.
void VAWorld::on_node_removed(Node *node)
{
    remove_primitive(node, false);
}

} // namespace va_godot
