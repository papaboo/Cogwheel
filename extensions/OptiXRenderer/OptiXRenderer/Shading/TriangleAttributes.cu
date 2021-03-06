// Triangle attribute interpolation program.
// ------------------------------------------------------------------------------------------------
// Copyright (C) Bifrost. See AUTHORS.txt for authors.
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#include <OptiXRenderer/Types.h>

#include <optix.h>
#include <optixu/optixu_math_namespace.h>

using namespace OptiXRenderer;
using namespace optix;

rtDeclareVariable(int, mesh_flags, , );
rtBuffer<uint3> index_buffer;
rtBuffer<VertexGeometry> geometry_buffer;
rtBuffer<float2> texcoord_buffer;

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float2, texcoord, attribute texcoord, );

//-------------------------------------------------------------------------------------------------
// Attribute program for triangle meshes.
//-------------------------------------------------------------------------------------------------
RT_PROGRAM void interpolate_attributes() {
    const uint3 vertex_indices = index_buffer[rtGetPrimitiveIndex()];
    const VertexGeometry g0 = geometry_buffer[vertex_indices.x];
    const VertexGeometry g1 = geometry_buffer[vertex_indices.y];
    const VertexGeometry g2 = geometry_buffer[vertex_indices.z];
    
    geometric_normal = normalize(cross(g1.position - g0.position, g2.position - g0.position));

    const float2 barycentrics = rtGetTriangleBarycentrics();

    if (mesh_flags & MeshFlags::Normals)
        shading_normal = g1.normal.decode_unnormalized() * barycentrics.x + g2.normal.decode_unnormalized() * barycentrics.y +
                         g0.normal.decode_unnormalized() * (1.0f - barycentrics.x - barycentrics.y);
    else
        shading_normal = geometric_normal;

    if (mesh_flags & MeshFlags::Texcoords) {
        const float2 texcoord0 = texcoord_buffer[vertex_indices.x];
        const float2 texcoord1 = texcoord_buffer[vertex_indices.y];
        const float2 texcoord2 = texcoord_buffer[vertex_indices.z];
        texcoord = texcoord1 * barycentrics.x + texcoord2 * barycentrics.y + texcoord0 * (1.0f - barycentrics.x - barycentrics.y);
    }
    else
        texcoord - make_float2(0.0f);
}
