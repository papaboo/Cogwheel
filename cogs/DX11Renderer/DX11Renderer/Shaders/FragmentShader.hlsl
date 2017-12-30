// Model fragment shaders.
// ---------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#include "DefaultShading.hlsl"
#include "LightSources.hlsl"
#include "SPTD.hlsl"

cbuffer scene_variables : register(b0) {
    float4x4 view_projection_matrix;
    float4 camera_position;
    float4 environment_tint; // .w component is 0 if an environment tex is not bound, otherwise positive.
};

cbuffer lights : register(b1) {
    int4 light_count;
    LightData light_data[12];
}

cbuffer material : register(b3) {
    DefaultShading material;
}

// TODO Potentially move to default shading and use the other precomputation sampler.
Texture2D sptd_ggx_fit_tex : register(t14);
SamplerState sptd_ggx_fit_sampler : register(s14);

struct BRDFPivotTransform {
    float3 pivot;
    float brdf_scale;
};

BRDFPivotTransform sptd_ggx_pivot(float roughness, float3 wo) {
    float2 sptd_ggx_fit_uv = float2(abs(wo.z), roughness);
    float4 pivot_params = sptd_ggx_fit_tex.Sample(sptd_ggx_fit_sampler, sptd_ggx_fit_uv);

    BRDFPivotTransform res;
    res.brdf_scale = pivot_params.w;
    float pivot_norm = pivot_params.x;
    float pivot_cos_theta = pivot_params.y;
    float pivot_sin_theta = -sqrt(1.0f - pivot_cos_theta * pivot_cos_theta);
    float3 pivot = pivot_norm * float3(pivot_sin_theta, 0, pivot_cos_theta);

    // Convert the pivot from local / wo space to tangent space. TODO Isn't there some faster way to move the vector than create the matrix explicitly?
    float3x3 basis;
    basis[0] = wo.z < 0.999f ? normalize(wo - float3(0, 0, wo.z)) : float3(1, 0, 0);
    basis[1] = cross(float3(0, 0, 1), basis[0]); // Has no effect. Looks like the whole thing can be inlined to use basis[0] and basis[2].z
    basis[2] = float3(0, 0, 1);
    res.pivot = mul(pivot, basis);
    return res;
}

BRDFPivotTransform sptd_lambert_pivot(float3 wo) {
    BRDFPivotTransform res;
    res.brdf_scale = 1.0f;
    float pivot_norm = 0.369589f;
    float pivot_theta = 0.0f;
    float3 pivot = pivot_norm * float3(sin(pivot_theta), 0, cos(pivot_theta));

    // Convert the pivot from local / wo space to tangent space. TODO Use TBN or perhaps inline. Isn't there some faster way to move the vector than create the matrix explicitly?
    float3x3 basis;
    basis[0] = wo.z < 0.999f ? normalize(wo - float3(0, 0, wo.z)) : float3(1, 0, 0);
    basis[1] = cross(float3(0, 0, 1), basis[0]);
    basis[2] = float3(0, 0, 1);
    res.pivot = mul(pivot, basis);
    return res;
}

float3 find_reflection_point(float3 p0, float3 p1) {
    float3 v = p1 - p0;
    float3 m = p0 + v / (abs(p1.z) + abs(p0.z)) * abs(p0.z); // TODO Only needed as long as the direction to light and camera can be on opposite sides.
    return float3(m.x, m.y, 0.0f);
}

float3 elongated_highlight_offset(float3 direction_to_camera, float3 direction_to_light, float elongation) {
    float3 perfect_reflection_point = find_reflection_point(direction_to_camera, direction_to_light); // TODO inline to reuse camera_to_light.
    float2 perfect_reflection_point_2D = perfect_reflection_point.xy;

    float2 bitangent = normalize(direction_to_light.xy - direction_to_camera.xy);
    float2 tangent = float2(-bitangent.y, bitangent.x);

    float2 delta_x = tangent * dot(perfect_reflection_point_2D, tangent);
    float2 delta_y = bitangent * dot(perfect_reflection_point_2D, bitangent);
    float2 warped_reflection_point = perfect_reflection_point_2D - delta_x * elongation - delta_y / elongation;
    return float3(warped_reflection_point, 0.0);
}
    
struct PixelInput {
    float4 position : SV_POSITION;
    float4 world_position : WORLD_POSITION;
    float4 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

float3 integration(PixelInput input) {
    float3 normal = normalize(input.normal.xyz);

    // Apply IBL
    float3 wo = camera_position.xyz - input.world_position.xyz;
    float3 _wo = camera_position.xyz - input.world_position.xyz;
    float3 wi = -reflect(normalize(wo), normal);
    float3 radiance = environment_tint.rgb * material.IBL(normal, wi, input.texcoord);

    float3x3 world_to_shading_TBN = create_TBN(normal);
    wo = normalize(mul(world_to_shading_TBN, wo));

    // Compute GGX SPTD params
    BRDFPivotTransform ggx_sptd = sptd_ggx_pivot(material.roughness(), wo);
    Cone ggx_sptd_cap = SPTD::pivot_transform(Cone::make(float3(0.0f, 0.0f, 1.0f), 0.0f), ggx_sptd.pivot);

    for (int l = 0; l < light_count.x; ++l) {
        LightData light = light_data[l];
        LightSample light_sample = sample_light(light_data[l], input.world_position.xyz);

        bool is_sphere_light = light.type() == LightType::Sphere && light.sphere_radius() > 0.0f;
        if (is_sphere_light) {
            // Sphere light in local space
            float3 sphere_position = mul(world_to_shading_TBN, light.sphere_position() - input.world_position.xyz);
            Sphere local_sphere = Sphere::make(sphere_position, light.sphere_radius());
            Cone light_sphere_cap = SPTD::sphere_to_sphere_cap(local_sphere.position, local_sphere.radius);

            Cone hemisphere_sphere_cap = Cone::make(float3(0.0f, 0.0f, 1.0f), 0.0f);
            float3 centroid_of_union = SPTD::centroid_of_union(hemisphere_sphere_cap, light_sphere_cap);

            float3 diffuse_tint, specular_tint;
            material.evaluate_tints(wo, centroid_of_union, input.texcoord, diffuse_tint, specular_tint);

            { // Evaluate Lambert. // TODO Optimize by perhaps approximating the union solidangle and centroid.
              // TODO Combine solidangle and centroid calculations.
                float solidangle_of_light = SPTD::solidangle(light_sphere_cap);
                float solidangle_of_union = SPTD::solidangle_of_union(hemisphere_sphere_cap, light_sphere_cap);
                float light_radiance_scale = solidangle_of_union / solidangle_of_light;
                radiance += diffuse_tint * BSDFs::Lambert::evaluate() * abs(centroid_of_union.z) * light_sample.radiance * light_radiance_scale;
            }

            { // Evaluate GGX/microfacet.
                // Stretch highlight based on roughness and cos_theta.
                // TODO Use centroid for evaluation instead of light_sphere_cap.direction
                float3 direction_to_camera = wo * length(_wo);
                float3 direction_to_light = light_sphere_cap.direction * length(sphere_position);
                float3 halfway = normalize(wo + centroid_of_union);
                float elongation = 1.0 + 4.0 * material.roughness() * (1.0f - sqrt(abs(wo.z)));
                float3 intersection_offset = elongated_highlight_offset(direction_to_camera, direction_to_light, elongation);
                light_sphere_cap.direction = normalize(direction_to_light - intersection_offset);

                float3 adjusted_wo = normalize(direction_to_camera - intersection_offset);
                BRDFPivotTransform adjusted_ggx_sptd = sptd_ggx_pivot(material.roughness(), adjusted_wo);
                Cone adjusted_ggx_sptd_cap = SPTD::pivot_transform(Cone::make(float3(0.0f, 0.0f, 1.0f), 0.0f), adjusted_ggx_sptd.pivot);

                Cone ggx_light_sphere_cap = SPTD::pivot_transform(light_sphere_cap, adjusted_ggx_sptd.pivot);
                float light_solidangle = SPTD::solidangle_of_union(ggx_light_sphere_cap, adjusted_ggx_sptd_cap);
                float3 l = light.sphere_power() / (PI * sphere_surface_area(light.sphere_radius()));
                radiance += specular_tint * light_solidangle / (4.0f * PI) * l;
            }

        } else {
            // Apply regular delta lights
            float3 wi = mul(world_to_shading_TBN, light_sample.direction_to_light);
            float3 f = material.evaluate(wo, wi, input.texcoord);
            radiance += f * light_sample.radiance * abs(wi.z);
        }
    }

    return radiance;
}

float4 opaque(PixelInput input) : SV_TARGET{
    // NOTE There may be a performance cost associated with having a potential discard, so we should probably have a separate pixel shader for cutouts.
    float coverage = material.coverage(input.texcoord);
    if (coverage < 0.33f)
        discard;

    return float4(integration(input), 1.0f);
}

float4 transparent(PixelInput input) : SV_TARGET{
    float coverage = material.coverage(input.texcoord);
    return float4(integration(input), coverage);
}