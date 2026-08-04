#pragma once

#include <cfloat>
#include <vector>

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

inline VAVector ToVAudio(const Vector3 &v)
{
    return vaVectorCreate(v.x, v.y, v.z);
}

inline Vector3 FromVAudio(const VAVector &v)
{
    return Vector3(v.x, v.y, v.z);
}

inline VAMatrix ToVAudio(const Transform3D &transform)
{
    const Basis &basis = transform.basis;
    const Vector3 &origin = transform.origin;

    Vector3 basis_x = basis.get_column(0);
    Vector3 basis_y = basis.get_column(1);
    Vector3 basis_z = basis.get_column(2);

    return vaMatrixCreate(
        basis_x.x, basis_x.y, basis_x.z, 0.0f,
        basis_y.x, basis_y.y, basis_y.z, 0.0f,
        basis_z.x, basis_z.y, basis_z.z, 0.0f,
        origin.x,  origin.y,  origin.z,  1.0f);
}

// Remove the scale from a matrix (most vaudio primitives only support rotation/translation, not scale)
inline Transform3D RemoveScale(const Transform3D &transform, Vector3 &out_scale)
{
    out_scale = transform.basis.get_scale();
    return Transform3D(transform.basis.orthonormalized(), transform.origin);
}

// Flattens every surface of a Godot Mesh into a flat CW-wound triangle vertex list for a VAMeshPrimitive, computing the local-space AABB along the way.
inline std::vector<VAVector> ConvertMeshToVAudio(const Ref<Mesh> &mesh, VAVector &out_min, VAVector &out_max)
{
    std::vector<VAVector> vertices;

    Vector3 min(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    auto add_triangle = [&](const PackedVector3Array &normals, int index, Vector3 v0, Vector3 v1, Vector3 v2)
    {
        if (normals.size() > 0 && index < normals.size())
        {
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 calculated_normal = edge1.cross(edge2).normalized();
            Vector3 mesh_normal = normals[index];

            if (calculated_normal.dot(mesh_normal) < 0)
            {
                // Flip winding to match the mesh's own normal direction.
                Vector3 temp = v1;
                v1 = v2;
                v2 = temp;
            }
        }

        // Godot uses CCW winding; VAudio expects CW - swap v1/v2 to convert.
        vertices.push_back(ToVAudio(v0));
        vertices.push_back(ToVAudio(v2));
        vertices.push_back(ToVAudio(v1));

        min.x = MIN(min.x, MIN(v0.x, MIN(v1.x, v2.x)));
        min.y = MIN(min.y, MIN(v0.y, MIN(v1.y, v2.y)));
        min.z = MIN(min.z, MIN(v0.z, MIN(v1.z, v2.z)));
        max.x = MAX(max.x, MAX(v0.x, MAX(v1.x, v2.x)));
        max.y = MAX(max.y, MAX(v0.y, MAX(v1.y, v2.y)));
        max.z = MAX(max.z, MAX(v0.z, MAX(v1.z, v2.z)));
    };

    int surface_count = mesh->get_surface_count();
    for (int surface_index = 0; surface_index < surface_count; surface_index++)
    {
        Array arrays = mesh->surface_get_arrays(surface_index);
        if (arrays.is_empty())
        {
            continue;
        }

        PackedVector3Array surface_vertices = arrays[Mesh::ARRAY_VERTEX];
        if (surface_vertices.is_empty())
        {
            continue;
        }

        Variant normals_variant = arrays[Mesh::ARRAY_NORMAL];
        PackedVector3Array normals;
        if (normals_variant.get_type() != Variant::NIL)
        {
            normals = normals_variant;
        }

        Variant indices_variant = arrays[Mesh::ARRAY_INDEX];
        if (indices_variant.get_type() == Variant::NIL)
        {
            // No index array - vertices are already in triangle order.
            for (int i = 0; i + 2 < surface_vertices.size(); i += 3)
            {
                add_triangle(normals, i, surface_vertices[i], surface_vertices[i + 1], surface_vertices[i + 2]);
            }
        }
        else
        {
            PackedInt32Array indices = indices_variant;
            for (int i = 0; i + 2 < indices.size(); i += 3)
            {
                int index0 = indices[i];
                int index1 = indices[i + 1];
                int index2 = indices[i + 2];

                add_triangle(normals, index0, surface_vertices[index0], surface_vertices[index1], surface_vertices[index2]);
            }
        }
    }

    if (vertices.empty())
    {
        min = Vector3();
        max = Vector3();
    }

    out_min = ToVAudio(min);
    out_max = ToVAudio(max);

    return vertices;
}
