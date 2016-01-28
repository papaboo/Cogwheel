// OptiX shading utilities.
// ---------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_SHADING_UTILS_H_
#define _OPTIXRENDERER_SHADING_UTILS_H_

#include <OptiXRenderer/Shading/Defines.h>

#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

namespace OptiXRenderer {
namespace Shading {

__inline_dev__ optix::float3 project_ray_direction(optix::float2 viewport_pos, 
                                                   const optix::float3& camera_position, 
                                                   const optix::Matrix4x4& inverted_view_projection_matrix) {
    using namespace optix;

    // Generate rays.
    float4 normalized_device_pos = make_float4(viewport_pos.x * 2.0f - 1.0f,
                                               1.0f - viewport_pos.y * 2.0f, // Inlined negate of the screen position.
                                               1.0f, 1.0f);

    float4 screenspace_world_pos = inverted_view_projection_matrix * normalized_device_pos;

    float3 ray_end = make_float3(screenspace_world_pos) / screenspace_world_pos.w;

    return normalize(ray_end - camera_position);
}

__inline_dev__ optix::float3 gammacorrect(const optix::float3& color, float gamma) {
    return optix::make_float3(pow(color.x, gamma),
                              pow(color.y, gamma),
                              pow(color.z, gamma));
}

__inline_dev__ optix::float4 gammacorrect(const optix::float4& color, float gamma) {
    return optix::make_float4(pow(color.x, gamma),
                              pow(color.y, gamma),
                              pow(color.z, gamma),
                              color.w);
}

}
}

#endif // _OPTIXRENDERER_SHADING_UTILS_H_