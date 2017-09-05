// OptiX shading utilities.
// ---------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_SHADING_UTILS_H_
#define _OPTIXRENDERER_SHADING_UTILS_H_

#include <OptiXRenderer/Defines.h>

#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

#include <cmath>

namespace OptiXRenderer {

__inline_dev__ optix::float3 project_ray_direction(optix::float2 viewport_pos, 
                                                   const optix::float3& camera_position, 
                                                   const optix::Matrix4x4& inverted_view_projection_matrix) {
    using namespace optix;

    float4 normalized_projected_pos = make_float4(viewport_pos.x * 2.0f - 1.0f,
                                                  viewport_pos.y * 2.0f - 1.0f,
                                                  -1.0f, 1.0f);

    float4 projected_world_pos = inverted_view_projection_matrix * normalized_projected_pos;

    float3 ray_origin = make_float3(projected_world_pos) / projected_world_pos.w;

    return normalize(ray_origin - camera_position);
}

// Computes a tangent and bitangent that together with the normal creates an orthonormal bases.
// Building an Orthonormal Basis, Revisited, Duff et al.
// http://jcgt.org/published/0006/01/01/paper.pdf
__inline_all__ static void compute_tangents(const optix::float3& normal,
                                            optix::float3& tangent, optix::float3& bitangent) {
    using namespace optix;

    float sign = copysignf(1.0f, normal.z);
    const float a = -1.0f / (sign + normal.z);
    const float b = normal.x * normal.y * a;
    tangent = { 1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x };
    bitangent = { b, sign + normal.y * normal.y * a, -normal.y };
}

__inline_all__ optix::float3 clamp_light_contribution_by_path_PDF(const optix::float3& radiance, float path_PDF, int accumulations) {
#if PATH_PDF_FIREFLY_FILTER
    float contribution = optix::luminance(radiance);
    float max_contribution = (1.0f / (1.0f - path_PDF)) - 1.0f;
    max_contribution *= (accumulations + 1);
    max_contribution += 1.0f;
    return radiance * fminf(max_contribution / contribution, 1.0f);
#else
    return radiance;
#endif
}

#if (defined(__CUDACC__) || defined(__CUDABE__))
__inline_dev__ optix::float4 half_to_float(const optix::ushort4& xyzw) {
    return optix::make_float4(__half2float(xyzw.x), __half2float(xyzw.y), __half2float(xyzw.z), __half2float(xyzw.w));
}

__inline_dev__ optix::ushort4 float_to_half(const optix::float4& xyzw) {
    return optix::make_ushort4(__float2half_rn(xyzw.x), __float2half_rn(xyzw.y), __float2half_rn(xyzw.z), __float2half_rn(xyzw.w));
}
#endif

//-----------------------------------------------------------------------------
// Math utils
//-----------------------------------------------------------------------------

__inline_all__ float average(const optix::float3& v) {
    return (v.x + v.y + v.z) / 3.0f;
}

__inline_all__ float heaviside(float v) {
    return v >= 0.0f ? 1.0f : 0.0f;
}

__inline_all__ float sign(float v) {
    return v >= 0.0f ? 1.0f : -1.0f;
}

__inline_all__ float sum(const optix::float3& v) {
    return v.x + v.y + v.z;
}

__inline_all__ optix::float3 gammacorrect(const optix::float3& color, float gamma) {
    return optix::make_float3(pow(color.x, gamma),
                              pow(color.y, gamma),
                              pow(color.z, gamma));
}

__inline_all__ optix::float4 gammacorrect(const optix::float4& color, float gamma) {
    return optix::make_float4(pow(color.x, gamma),
                              pow(color.y, gamma),
                              pow(color.z, gamma),
                              color.w);
}

__inline_all__ bool is_PDF_valid(float PDF) {
    return PDF > 0.000001f;
}

// Insert a 0 bit in between each of the 16 low bits of v.
__inline_all__ unsigned int part_by_1(unsigned int v) {
    v &= 0x0000ffff;                 // v = ---- ---- ---- ---- fedc ba98 7654 3210
    v = (v ^ (v << 8)) & 0x00ff00ff; // v = ---- ---- fedc ba98 ---- ---- 7654 3210
    v = (v ^ (v << 4)) & 0x0f0f0f0f; // v = ---- fedc ---- ba98 ---- 7654 ---- 3210
    v = (v ^ (v << 2)) & 0x33333333; // v = --fe --dc --ba --98 --76 --54 --32 --10
    v = (v ^ (v << 1)) & 0x55555555; // v = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    return v;
}

__inline_all__ unsigned int morton_encode(unsigned int x, unsigned int y) {
    return part_by_1(y) | (part_by_1(x) << 1);
}

__inline_all__ static float pow5(float x) {
    float xx = x * x;
    return xx * xx * x;
}

__inline_all__ float schlick_fresnel(float incident_specular, float abs_cos_theta) {
    return incident_specular + (1.0f - incident_specular) * pow5(optix::fmaxf(0.0f, 1.0f - abs_cos_theta));
}

__inline_all__ optix::float2 direction_to_latlong_texcoord(const optix::float3& direction) {
    float u = (atan2f(direction.z, direction.x) + PIf) * 0.5f / PIf;
    float v = (asinf(direction.y) + PIf * 0.5f) / PIf;
    return optix::make_float2(u, v);
}

__inline_all__ optix::float3 latlong_texcoord_to_direction(const optix::float2& uv) {
    float phi = uv.x * 2.0f * PIf;
    float theta = uv.y * PIf;
    float sin_theta = sinf(theta);
    return -optix::make_float3(sin_theta * cosf(phi), cosf(theta), sin_theta * sinf(phi));
}

//-----------------------------------------------------------------------------
// Trigonometri utils
//-----------------------------------------------------------------------------

__inline_all__ float cos_theta(optix::float3 w) { return w.z; }
__inline_all__ float cos2_theta(optix::float3 w) { return w.z * w.z; }
__inline_all__ float abs_cos_theta(optix::float3 w) { return abs(w.z); }

__inline_all__ float sin2_theta(optix::float3 w) { return fmaxf(0.0f, 1.0f - cos2_theta(w)); }
__inline_all__ float sin_theta(optix::float3 w) { return sqrt(sin2_theta(w)); }

__inline_all__ float tan_theta(optix::float3 w) { return sin_theta(w) / cos_theta(w); }
__inline_all__ float tan2_theta(optix::float3 w) { return sin2_theta(w) / cos2_theta(w); }

__inline_all__ float cos_phi(optix::float3 w) {
    float sin_theta_ = sin_theta(w);
    return (sin_theta_ == 0) ? 1.0f : optix::clamp(w.x / sin_theta_, -1.0f, 1.0f);
}
__inline_all__ float cos2_phi(optix::float3 w) { return cos_phi(w) * cos_phi(w); }

__inline_all__ float sin_phi(optix::float3 w) {
    float sin_theta_ = sin_theta(w);
    return (sin_theta_ == 0) ? 0.0f : optix::clamp(w.y / sin_theta_, -1.0f, 1.0f);
}
__inline_all__ float sin2_phi(optix::float3 w) { return sin_phi(w) * sin_phi(w); }

} // NS OptiXRenderer

#endif // _OPTIXRENDERER_SHADING_UTILS_H_