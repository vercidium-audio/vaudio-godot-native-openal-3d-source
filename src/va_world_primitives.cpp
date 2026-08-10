#include "va_world.h"

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world_boundary_shape3d.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_conversions.h"
#include "va_custom_material.h"
#include "va_engine_util.h"

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

// Names of the built-in VAMaterialType values, indexed by VAMaterialType. The single source of
// truth for both VAWorld::get_material's matching below and get_builtin_material_names/
// get_builtin_material_names_array, which expose this list to the "Vercidium Audio" editor plugin.
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

    VAResult material_result = vaPrismPrimitiveSetMaterial(prim, material);
    if (material_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(CSGBox3D) failed to set material for '",
            csg_box->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
    }

    VAVector size = ToVAudio(csg_box->get_size() * scale);
    vaPrismPrimitiveSetSize(prim, size);
    VAMatrix mat = ToVAudio(transform);
    vaPrismPrimitiveSetTransform(prim, &mat);

    VAResult add_result = vaWorldAddPrimitive_(world, prim);
    if (add_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(CSGBox3D) failed to add '",
            csg_box->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
        vaPrismPrimitiveDestroy(prim);
        return;
    }

    VAPrimitiveRef *ref = attach_watcher(csg_box, prim, VAPrimitiveKind::Prism, [this, csg_box, prim]()
    {
        Vector3 updated_scale;
        Transform3D updated_transform = RemoveScale(csg_box->get_global_transform(), updated_scale);
        VAVector updated_size = ToVAudio(csg_box->get_size() * updated_scale);

        // TransformWatcher only fires on an actual transform change, so
        // VA_UNCHANGED here just means this particular setter's value (e.g.
        // size when only rotation changed) happened to stay the same - not
        // an error.
        VAResult size_result = vaPrismPrimitiveSetSize(prim, updated_size);
        if (size_result != VA_SUCCESS && size_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGBox3D) failed to update size for '",
                csg_box->get_name(), "' (VAResult=", VAResultToString(size_result), ")");
        }

        VAMatrix updated_mat = ToVAudio(updated_transform);
        VAResult transform_result = vaPrismPrimitiveSetTransform(prim, &updated_mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGBox3D) failed to update transform for '",
                csg_box->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }
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

        VAResult material_result = vaConePrimitiveSetMaterial(prim, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGCylinder3D cone) failed to set material for '",
                csg_cylinder->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        vaConePrimitiveSetRadius(prim, csg_cylinder->get_radius());
        vaConePrimitiveSetHeight(prim, csg_cylinder->get_height());
        VAMatrix mat = ToVAudio(offset_transform);
        vaConePrimitiveSetTransform(prim, &mat);

        VAResult add_result = vaWorldAddPrimitive_(world, prim);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGCylinder3D cone) failed to add '",
                csg_cylinder->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaConePrimitiveDestroy(prim);
            return;
        }

        VAPrimitiveRef *ref = attach_watcher(csg_cylinder, prim, VAPrimitiveKind::Cone, [this, csg_cylinder, prim]()
        {
            Transform3D updated_offset = csg_cylinder->get_global_transform().translated_local(Vector3(0, -csg_cylinder->get_height() / 2, 0));

            // TransformWatcher only fires on an actual transform change, so
            // VA_UNCHANGED here just means this setter's own value happened
            // to stay the same - not an error.
            VAResult radius_result = vaConePrimitiveSetRadius(prim, csg_cylinder->get_radius());
            if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D cone) failed to update radius for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
            }

            VAResult height_result = vaConePrimitiveSetHeight(prim, csg_cylinder->get_height());
            if (height_result != VA_SUCCESS && height_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D cone) failed to update height for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(height_result), ")");
            }

            VAMatrix updated_mat = ToVAudio(updated_offset);
            VAResult transform_result = vaConePrimitiveSetTransform(prim, &updated_mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D cone) failed to update transform for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
        });

        csg_cylinder->set_meta(PrimitiveMetaKey(), ref);
    }
    else
    {
        VACylinderPrimitive *prim = vaCylinderPrimitiveCreate();

        VAResult material_result = vaCylinderPrimitiveSetMaterial(prim, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGCylinder3D) failed to set material for '",
                csg_cylinder->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        vaCylinderPrimitiveSetRadius(prim, csg_cylinder->get_radius());
        vaCylinderPrimitiveSetLength(prim, csg_cylinder->get_height());
        VAMatrix mat = ToVAudio(csg_cylinder->get_global_transform());
        vaCylinderPrimitiveSetTransform(prim, &mat);

        VAResult add_result = vaWorldAddPrimitive_(world, prim);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGCylinder3D) failed to add '",
                csg_cylinder->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaCylinderPrimitiveDestroy(prim);
            return;
        }

        VAPrimitiveRef *ref = attach_watcher(csg_cylinder, prim, VAPrimitiveKind::Cylinder, [this, csg_cylinder, prim]()
        {
            // TransformWatcher only fires on an actual transform change, so
            // VA_UNCHANGED here just means this setter's own value happened
            // to stay the same - not an error.
            VAResult radius_result = vaCylinderPrimitiveSetRadius(prim, csg_cylinder->get_radius());
            if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D) failed to update radius for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
            }

            VAResult length_result = vaCylinderPrimitiveSetLength(prim, csg_cylinder->get_height());
            if (length_result != VA_SUCCESS && length_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D) failed to update length for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(length_result), ")");
            }

            VAMatrix updated_mat = ToVAudio(csg_cylinder->get_global_transform());
            VAResult transform_result = vaCylinderPrimitiveSetTransform(prim, &updated_mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::create_primitive(CSGCylinder3D) failed to update transform for '",
                    csg_cylinder->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
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

    VAResult material_result = vaSpherePrimitiveSetMaterial(prim, material);
    if (material_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(CSGSphere3D) failed to set material for '",
            csg_sphere->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
    }

    vaSpherePrimitiveSetCenter(prim, ToVAudio(csg_sphere->get_global_transform().origin));
    vaSpherePrimitiveSetRadius(prim, csg_sphere->get_radius());

    VAResult add_result = vaWorldAddPrimitive_(world, prim);
    if (add_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(CSGSphere3D) failed to add '",
            csg_sphere->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
        vaSpherePrimitiveDestroy(prim);
        return;
    }

    VAPrimitiveRef *ref = attach_watcher(csg_sphere, prim, VAPrimitiveKind::Sphere, [this, csg_sphere, prim]()
    {
        // TransformWatcher only fires on an actual transform change, so
        // VA_UNCHANGED here just means this setter's own value happened to
        // stay the same - not an error.
        VAResult center_result = vaSpherePrimitiveSetCenter(prim, ToVAudio(csg_sphere->get_global_transform().origin));
        if (center_result != VA_SUCCESS && center_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGSphere3D) failed to update center for '",
                csg_sphere->get_name(), "' (VAResult=", VAResultToString(center_result), ")");
        }

        VAResult radius_result = vaSpherePrimitiveSetRadius(prim, csg_sphere->get_radius());
        if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CSGSphere3D) failed to update radius for '",
                csg_sphere->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
        }
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

    // get_global_transform() bakes in any non-uniform scale inherited from
    // parent Node3Ds, not just this CollisionShape3D's own local scale - so
    // RemoveScale (which reads it back off the basis) is what must drive
    // radius/length, and orthonormalized_transform (scale-free basis) is
    // what must go to Set*Transform - passing a non-uniformly-scaled basis
    // there is what caused VA_INVALID_VALUE.
    Vector3 scale;
    Transform3D orthonormalized_transform = RemoveScale(global_transform, scale);

    void *prim = nullptr;
    VAPrimitiveKind kind;

    if (Ref<BoxShape3D> box = shape; box.is_valid())
    {
        Vector3 box_scale;
        Transform3D box_transform = RemoveScale(global_transform, box_scale);

        VAPrismPrimitive *p = vaPrismPrimitiveCreate();

        VAResult material_result = vaPrismPrimitiveSetMaterial(p, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D box) failed to set material for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        VAVector size = ToVAudio(box->get_size() * box_scale);
        VAResult size_result = vaPrismPrimitiveSetSize(p, size);
        if (size_result != VA_SUCCESS && size_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D box) failed to set size for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(size_result), ")");
        }

        VAMatrix mat = ToVAudio(box_transform);
        VAResult transform_result = vaPrismPrimitiveSetTransform(p, &mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D box) failed to set transform for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }

        VAResult add_result = vaWorldAddPrimitive_(world, p);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D box) failed to add '",
                collision_shape->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaPrismPrimitiveDestroy(p);
            return;
        }

        prim = p;
        kind = VAPrimitiveKind::Prism;
    }
    else if (Ref<SphereShape3D> sphere = shape; sphere.is_valid())
    {
        VASpherePrimitive *p = vaSpherePrimitiveCreate();

        VAResult material_result = vaSpherePrimitiveSetMaterial(p, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D sphere) failed to set material for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        VAResult center_result = vaSpherePrimitiveSetCenter(p, ToVAudio(position));
        if (center_result != VA_SUCCESS && center_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D sphere) failed to set center for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(center_result), ")");
        }

        VAResult radius_result = vaSpherePrimitiveSetRadius(p, sphere->get_radius() * scale.x);
        if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D sphere) failed to set radius for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
        }

        VAResult add_result = vaWorldAddPrimitive_(world, p);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D sphere) failed to add '",
                collision_shape->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaSpherePrimitiveDestroy(p);
            return;
        }

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

        VAResult material_result = vaCapsulePrimitiveSetMaterial(p, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D capsule) failed to set material for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        VAResult radius_result = vaCapsulePrimitiveSetRadius(p, capsule->get_radius() * scale.x);
        if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D capsule) failed to set radius for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
        }

        VAResult length_result = vaCapsulePrimitiveSetLength(p, cylinder_length * scale.y);
        if (length_result != VA_SUCCESS && length_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D capsule) failed to set length for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(length_result), ")");
        }

        VAMatrix mat = ToVAudio(orthonormalized_transform);
        VAResult transform_result = vaCapsulePrimitiveSetTransform(p, &mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D capsule) failed to set transform for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }

        VAResult add_result = vaWorldAddPrimitive_(world, p);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D capsule) failed to add '",
                collision_shape->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaCapsulePrimitiveDestroy(p);
            return;
        }

        prim = p;
        kind = VAPrimitiveKind::Capsule;
    }
    else if (Ref<CylinderShape3D> cylinder = shape; cylinder.is_valid())
    {
        VACylinderPrimitive *p = vaCylinderPrimitiveCreate();

        VAResult material_result = vaCylinderPrimitiveSetMaterial(p, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D cylinder) failed to set material for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        VAResult radius_result = vaCylinderPrimitiveSetRadius(p, cylinder->get_radius() * scale.x);
        if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D cylinder) failed to set radius for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
        }

        VAResult length_result = vaCylinderPrimitiveSetLength(p, cylinder->get_height() * scale.y);
        if (length_result != VA_SUCCESS && length_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D cylinder) failed to set length for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(length_result), ")");
        }

        VAMatrix mat = ToVAudio(orthonormalized_transform);
        VAResult transform_result = vaCylinderPrimitiveSetTransform(p, &mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D cylinder) failed to set transform for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }

        VAResult add_result = vaWorldAddPrimitive_(world, p);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D cylinder) failed to add '",
                collision_shape->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaCylinderPrimitiveDestroy(p);
            return;
        }

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

        VAResult material_result = vaPlanePrimitiveSetMaterial(p, material);
        if (material_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D world boundary) failed to set material for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(material_result), ")");
        }

        // Use the max world size to ensure the plane covers the raytracing
        // scene, *2 in case the plane is positioned in the corner of the world.
        VAResult width_result = vaPlanePrimitiveSetWidth(p, world_magnitude * 2);
        if (width_result != VA_SUCCESS && width_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D world boundary) failed to set width for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(width_result), ")");
        }

        VAResult height_result = vaPlanePrimitiveSetHeight(p, world_magnitude * 2);
        if (height_result != VA_SUCCESS && height_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D world boundary) failed to set height for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(height_result), ")");
        }

        VAMatrix mat = ToVAudio(plane_transform);
        VAResult transform_result = vaPlanePrimitiveSetTransform(p, &mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D world boundary) failed to set transform for '",
                collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }

        VAResult add_result = vaWorldAddPrimitive_(world, p);
        if (add_result != VA_SUCCESS)
        {
            VA_ERROR(
                "VAWorld::create_primitive(CollisionShape3D world boundary) failed to add '",
                collision_shape->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
            vaPlanePrimitiveDestroy(p);
            return;
        }

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
        VA_WARN("MeshInstance3D ", mesh_instance->get_name(), " will not affect raytracing as it has no mesh");
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
    VAResult create_result = vaMeshPrimitiveCreate(material, triangles.data(), (int)triangles.size(), min, max, &mat, &prim);
    if (create_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(MeshInstance3D) failed to create the mesh primitive for '",
            mesh_instance->get_name(), "' (VAResult=", VAResultToString(create_result), ")");
        return;
    }

    // TODO - make this a metadata / inspector flag in Godot.
    VAResult permeation_result = vaMeshPrimitiveSetSupports3DPermeation(prim, true);
    if (permeation_result != VA_SUCCESS && permeation_result != VA_UNCHANGED)
    {
        VA_ERROR(
            "VAWorld::create_primitive(MeshInstance3D) failed to set 3D permeation support for '",
            mesh_instance->get_name(), "' (VAResult=", VAResultToString(permeation_result), ")");
    }

    VAResult add_result = vaWorldAddPrimitive_(world, prim);
    if (add_result != VA_SUCCESS)
    {
        VA_ERROR(
            "VAWorld::create_primitive(MeshInstance3D) failed to add '",
            mesh_instance->get_name(), "' to the world (VAResult=", VAResultToString(add_result), ")");
        vaMeshPrimitiveDestroy(prim);
        return;
    }

    VAPrimitiveRef *ref = attach_watcher(mesh_instance, prim, VAPrimitiveKind::Mesh, [this, mesh_instance, prim]()
    {
        VAMatrix updated_mat = ToVAudio(mesh_instance->get_global_transform());

        // TransformWatcher only fires on an actual transform change, so
        // VA_UNCHANGED is not expected here, but isn't an error either.
        VAResult transform_result = vaMeshPrimitiveSetTransform(prim, &updated_mat);
        if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
        {
            VA_ERROR(
                "VAWorld::create_primitive(MeshInstance3D) failed to update transform for '",
                mesh_instance->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
        }
    });

    mesh_instance->set_meta(PrimitiveMetaKey(), ref);
}

void VAWorld::update_collision_shape_primitive(CollisionShape3D *collision_shape, VAPrimitiveRef *ref)
{
    Transform3D global_transform = collision_shape->get_global_transform();

    // See create_primitive(CollisionShape3D)'s comment: scale here must be
    // read back off the global transform's basis (which bakes in inherited
    // parent scale too), not collision_shape->get_scale() alone, and
    // orthonormalized_transform (not global_transform) is what's safe to
    // pass to Set*Transform.
    Vector3 scale;
    Transform3D orthonormalized_transform = RemoveScale(global_transform, scale);
    Ref<Shape3D> shape = collision_shape->get_shape();

    switch (ref->kind)
    {
        case VAPrimitiveKind::Sphere:
        {
            VASpherePrimitive *p = (VASpherePrimitive *)ref->primitive;
            Ref<SphereShape3D> sphere = shape;

            // TransformWatcher only fires on an actual transform change, so
            // VA_UNCHANGED here just means this setter's own value happened
            // to stay the same - not an error.
            VAResult center_result = vaSpherePrimitiveSetCenter(p, ToVAudio(global_transform.origin));
            if (center_result != VA_SUCCESS && center_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(sphere) failed to update center for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(center_result), ")");
            }

            VAResult radius_result = vaSpherePrimitiveSetRadius(p, sphere->get_radius() * scale.x);
            if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(sphere) failed to update radius for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
            }
            break;
        }
        case VAPrimitiveKind::Prism:
        {
            VAPrismPrimitive *p = (VAPrismPrimitive *)ref->primitive;
            Ref<BoxShape3D> box = shape;
            Vector3 box_scale;
            Transform3D box_transform = RemoveScale(global_transform, box_scale);
            VAVector size = ToVAudio(box->get_size() * box_scale);

            VAResult size_result = vaPrismPrimitiveSetSize(p, size);
            if (size_result != VA_SUCCESS && size_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(box) failed to update size for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(size_result), ")");
            }

            VAMatrix mat = ToVAudio(box_transform);
            VAResult transform_result = vaPrismPrimitiveSetTransform(p, &mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(box) failed to update transform for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
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

            VAResult radius_result = vaCapsulePrimitiveSetRadius(p, capsule->get_radius() * scale.x);
            if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(capsule) failed to update radius for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
            }

            VAResult length_result = vaCapsulePrimitiveSetLength(p, cylinder_length * scale.y);
            if (length_result != VA_SUCCESS && length_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(capsule) failed to update length for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(length_result), ")");
            }

            VAMatrix mat = ToVAudio(orthonormalized_transform);
            VAResult transform_result = vaCapsulePrimitiveSetTransform(p, &mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(capsule) failed to update transform for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
            break;
        }
        case VAPrimitiveKind::Cylinder:
        {
            VACylinderPrimitive *p = (VACylinderPrimitive *)ref->primitive;
            Ref<CylinderShape3D> cylinder = shape;

            VAResult radius_result = vaCylinderPrimitiveSetRadius(p, cylinder->get_radius() * scale.x);
            if (radius_result != VA_SUCCESS && radius_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(cylinder) failed to update radius for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(radius_result), ")");
            }

            VAResult length_result = vaCylinderPrimitiveSetLength(p, cylinder->get_height() * scale.y);
            if (length_result != VA_SUCCESS && length_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(cylinder) failed to update length for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(length_result), ")");
            }

            VAMatrix mat = ToVAudio(orthonormalized_transform);
            VAResult transform_result = vaCylinderPrimitiveSetTransform(p, &mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(cylinder) failed to update transform for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
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
            VAResult transform_result = vaPlanePrimitiveSetTransform(p, &mat);
            if (transform_result != VA_SUCCESS && transform_result != VA_UNCHANGED)
            {
                VA_ERROR(
                    "VAWorld::update_collision_shape_primitive(plane) failed to update transform for '",
                    collision_shape->get_name(), "' (VAResult=", VAResultToString(transform_result), ")");
            }
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
            create_primitive(csg_box, material);
        else if (CSGCylinder3D *csg_cylinder = Object::cast_to<CSGCylinder3D>(node))
            create_primitive(csg_cylinder, material);
        else if (CSGSphere3D *csg_sphere = Object::cast_to<CSGSphere3D>(node))
            create_primitive(csg_sphere, material);
        else if (CollisionShape3D *collision_shape = Object::cast_to<CollisionShape3D>(node))
            create_primitive(collision_shape, material);
        else if (MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(node))
            create_primitive(mesh_instance, material);
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

            // vaWorldRemovePrimitive_ failing with VA_NOT_ADDED_TO_WORLD means
            // our bookkeeping (VAPrimitiveRef) disagrees with the SDK about
            // this primitive's membership - still proceed to Destroy so we
            // don't leak the primitive, but surface the mismatch.
            switch (ref->kind)
            {
                case VAPrimitiveKind::Prism:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(prism) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaPrismPrimitiveDestroy((VAPrismPrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(prism) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Cylinder:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(cylinder) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaCylinderPrimitiveDestroy((VACylinderPrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(cylinder) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Cone:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(cone) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaConePrimitiveDestroy((VAConePrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(cone) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Sphere:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(sphere) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaSpherePrimitiveDestroy((VASpherePrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(sphere) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Capsule:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(capsule) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaCapsulePrimitiveDestroy((VACapsulePrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(capsule) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Plane:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(plane) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaPlanePrimitiveDestroy((VAPlanePrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(plane) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
                case VAPrimitiveKind::Mesh:
                {
                    VAResult remove_result = vaWorldRemovePrimitive_(world, ref->primitive);
                    if (remove_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(mesh) failed to remove '",
                            node->get_name(), "' from the world (VAResult=", VAResultToString(remove_result), ")");
                    }

                    VAResult destroy_result = vaMeshPrimitiveDestroy((VAMeshPrimitive *)ref->primitive);
                    if (destroy_result != VA_SUCCESS)
                    {
                        VA_ERROR(
                            "VAWorld::remove_primitive(mesh) failed to destroy '",
                            node->get_name(), "' (VAResult=", VAResultToString(destroy_result), ")");
                    }
                    break;
                }
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

void VAWorld::validate_materials_in_editor(Node *node)
{
    // get_material already pushes a warning for an unrecognized string - just
    // need to trigger it for every node carrying the metadata.
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
    // Scans from get_tree()->get_root() rather than get_current_scene() - a
    // VAWorld's own subtree isn't always get_current_scene() itself (e.g.
    // truck_town's car_select.gd adds its level scene as a sibling of the
    // current scene via get_parent().add_child(town), so get_current_scene()
    // would stay pointed at the menu and miss every primitive already baked
    // into the level's .tscn). See va_world_lookup.h's find_va_world for the
    // same fix applied to VAWorld discovery.
    Node *root = get_tree()->get_root();
    if (!root)
    {
        return;
    }

    TypedArray<Node> children = root->get_children();
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
    // Godot's node duplication (e.g. editor copy-paste) copies metadata
    // along with it, so a freshly duplicated node can arrive here already
    // carrying its source node's PrimitiveMetaKey ref (same VAPrimitiveRef
    // instance, watcher and all). create_primitive's has_meta guard would
    // then skip it as "already added", leaving the duplicate without its
    // own primitive/watcher entirely. Strip any inherited primitive meta
    // before add_primitive runs so duplicates always get created fresh.
    if (node->has_meta(PrimitiveMetaKey()))
    {
        node->remove_meta(PrimitiveMetaKey());
    }

    add_primitive(node, VAMaterialAir, false);
}

// This fires for the new parent node AND each of its child nodes separately -
// child nodes are invoked first.
void VAWorld::on_node_removed(Node *node)
{
    remove_primitive(node, false);
}

} // namespace va_godot
