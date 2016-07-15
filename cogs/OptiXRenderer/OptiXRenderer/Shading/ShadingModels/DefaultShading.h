// OptiX renderer default shading model.
// ---------------------------------------------------------------------------
// Copyright (C) 2015-2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_SHADING_MODEL_DEFAULT_SHADING_H_
#define _OPTIXRENDERER_SHADING_MODEL_DEFAULT_SHADING_H_

#include <OptiXRenderer/Shading/BSDFs/Lambert.h>
#include <OptiXRenderer/Shading/BSDFs/GGX.h>
#include <OptiXRenderer/Utils.h>

#if GPU_DEVICE
rtDeclareVariable(unsigned int, default_shading_rho_texture_ID, , );
#endif

namespace OptiXRenderer {
namespace Shading {
namespace ShadingModels {

// ---------------------------------------------------------------------------
// The default shading material.
// Default shading consist of a diffuse base with a specular layer on top.
// The diffuse and specular contribution is weighted by a fresnel term.
// NOTE:
//  The fresnel term will cause the material to emit more than 100% energy 
//  at grazing angles when the material is very smooth. This is due to 
//  the specular component contributing almost 100%, because Fresnel will 
//  do little to reduce the contribution which is centered on the perfectly 
//  reflected directions. Meanwhile, across the rest ofe the hemisphere, 
//  the diffuse component will contribute by 1 - Fresnel, bringing the total 
//  contribution above 100%. Worst case seems to be 197%.
//  Possible solutions 
//  * Investigate samples where f * cos_theta / pdf is above one.
//    If all are one or below this should never be a problem.
//  * Investigating the Disney BRDF and check if they are energy conserving.
//  * Remove diffuse contribution such that the summed contribution equals one.
// ---------------------------------------------------------------------------
class DefaultShading {
private:
    const Material& m_material;
    optix::float3 m_base_tint;

    // TODO Implement Unreal 4 spherical gaussian and correct Fresnel as well. Move to Fresnel.h
    __inline_all__ static float schlick_fresnel(float incident_specular, float abs_cos_theta) {
        return incident_specular + (1.0f - incident_specular) * pow(optix::fmaxf(0.0f, 1.0f - abs_cos_theta), 5.0f);
    }

    // TODO Template with return type? Might be hell on the CPU.
    // TODO Move to utils.
    __inline_all__ static optix::float4 tex2D(unsigned int texture_ID, optix::float2 texcoord) {
#if GPU_DEVICE
        optix::float4 texel = optix::rtTex2D<optix::float4>(texture_ID, texcoord.x, texcoord.y);
        return texel;
#else
        return optix::make_float4(1.0f);
#endif
    }

    // Compute BSDF sampling probabilities based on their tinted weight.
    __inline_all__ static float compute_specular_probability(optix::float3 base_tint, optix::float3 specular_tint, float roughness, float specularity, float abs_cos_theta) {
        float diffuse_weight = sum(base_tint);
        float specular_weight = sum(specular_tint);

#if GPU_DEVICE
        // On the GPU we have access to a texture with preintegrated rho values.
        optix::float2 specular_diffuse_rho = optix::rtTex2D<optix::float2>(default_shading_rho_texture_ID, abs_cos_theta, roughness);
        float diffuse_rho = specular_diffuse_rho.x * (1.0f - specularity);
        diffuse_weight *= diffuse_rho;
        float specular_rho = optix::lerp(specular_diffuse_rho.y, 1.0f, specularity);
        specular_weight *= specular_rho;
#endif
        return specular_weight / (diffuse_weight + specular_weight);
    }

public:

    __inline_all__ DefaultShading(const Material& material)
        : m_material(material)
        , m_base_tint(material.base_tint) { }

    __inline_all__ DefaultShading(const Material& material, optix::float2 texcoord)
        : m_material(material)
    {
        m_base_tint = material.base_tint;
        if (material.base_tint_texture_ID)
            m_base_tint *= make_float3(tex2D(material.base_tint_texture_ID, texcoord));
    }

    __inline_all__ optix::float3 evaluate(optix::float3 wo, optix::float3 wi) const {
        using namespace optix;

        bool is_same_hemisphere = wi.z * wo.z >= 0.00000001f;
        if (!is_same_hemisphere)
            return make_float3(0.0f);

        // Flip directions if on the backside of the material.
        if (wi.z < 0.0f) {
            wi.z = -wi.z;
            wo.z = -wo.z;
        }

        float3 halfway = normalize(wo + wi);
        float specularity = lerp(m_material.specularity, 1.0f, m_material.metallic);
        float fresnel = schlick_fresnel(specularity, dot(wo, halfway));
        float3 specular_tint = lerp(make_float3(1.0f), m_base_tint, m_material.metallic);
        float alpha = BSDFs::GGX::alpha_from_roughness(m_material.base_roughness);
        float3 specular = specular_tint * (fresnel * BSDFs::GGX::evaluate(alpha, wo, wi, halfway));
        float3 diffuse = (1.0f - fresnel) * BSDFs::Lambert::evaluate(m_base_tint);
        return diffuse + specular;
    }

    __inline_all__ BSDFSample sample_one(const optix::float3& wo, const optix::float3& random_sample) const {
        using namespace optix;

        const float3 specular_tint = lerp(make_float3(1.0f), m_base_tint, m_material.metallic);
        float specularity = lerp(m_material.specularity, 1.0f, m_material.metallic);
        float abs_cos_theta = abs(wo.z);

        // Sample BSDFs based on the contribution of each BRDF.
        float specular_probability = compute_specular_probability(m_base_tint, specular_tint, m_material.base_roughness, specularity, abs_cos_theta);
        bool sample_specular = random_sample.z < specular_probability;

        // Sample selected BRDF.
        BSDFSample bsdf_sample;
        if (sample_specular) {
            float alpha = BSDFs::GGX::alpha_from_roughness(m_material.base_roughness);
            bsdf_sample = BSDFs::GGX::sample(specular_tint, alpha, wo, make_float2(random_sample));
            bsdf_sample.PDF *= specular_probability;
        } else {
            bsdf_sample = BSDFs::Lambert::sample(m_base_tint, make_float2(random_sample));
            bsdf_sample.PDF *= (1.0f - specular_probability);
        }

        // Apply Fresnel.
        optix::float3 halfway = normalize(wo + bsdf_sample.direction);
        float fresnel = schlick_fresnel(specularity, dot(wo, halfway));

        if (sample_specular)
            bsdf_sample.weight *= fresnel;
        else
            bsdf_sample.weight *= (1.0f - fresnel);

        return bsdf_sample;
    }

    __inline_all__ BSDFSample sample_all(const optix::float3& wo, const optix::float3& random_sample) const {
        using namespace optix;

        const float ggx_alpha = BSDFs::GGX::alpha_from_roughness(m_material.base_roughness);
        const float3 specular_tint = lerp(make_float3(1.0f), m_base_tint, m_material.metallic);
        float specularity = lerp(m_material.specularity, 1.0f, m_material.metallic);
        float abs_cos_theta = abs(wo.z);

        // Sample BSDFs based on the contribution of each BRDF.
        float specular_probability = compute_specular_probability(m_base_tint, specular_tint, m_material.base_roughness, specularity, abs_cos_theta);
        bool sample_specular = random_sample.z < specular_probability;

        // Sample selected BRDF.
        BSDFSample bsdf_sample;
        if (sample_specular) {
            bsdf_sample = BSDFs::GGX::sample(specular_tint, ggx_alpha, wo, make_float2(random_sample));
            bsdf_sample.PDF *= specular_probability;
        } else {
            bsdf_sample = BSDFs::Lambert::sample(m_base_tint, make_float2(random_sample));
            bsdf_sample.PDF *= (1.0f - specular_probability);
        }

        // Compute Fresnel.
        optix::float3 halfway = normalize(wo + bsdf_sample.direction);
        float fresnel = schlick_fresnel(specularity, dot(wo, halfway));

        // Apply fresnel and compute contribution of the rest of the material.
        if (sample_specular) {
            bsdf_sample.weight *= fresnel;

            // Evaluate diffuse layer as well.
            bsdf_sample.weight += (1.0f - fresnel) * BSDFs::Lambert::evaluate(m_base_tint, wo, bsdf_sample.direction);
            bsdf_sample.PDF += (1.0f - specular_probability) * BSDFs::Lambert::PDF(wo, bsdf_sample.direction);
        } else {
            bsdf_sample.weight *= (1.0f - fresnel);

            // Evaluate specular layer as well.
            bsdf_sample.weight += fresnel * BSDFs::GGX::evaluate(specular_tint, ggx_alpha, wo, bsdf_sample.direction);
            bsdf_sample.PDF += specular_probability * BSDFs::GGX::PDF(ggx_alpha, wo, bsdf_sample.direction);
        }

        return bsdf_sample;
    }

    __inline_all__ float PDF(const optix::float3& wo, const optix::float3& wi) const {
        using namespace optix;

        const float ggx_alpha = BSDFs::GGX::alpha_from_roughness(m_material.base_roughness);
        const float3 specular_tint = lerp(make_float3(1.0f), m_base_tint, m_material.metallic);
        float specularity = lerp(m_material.specularity, 1.0f, m_material.metallic);
        float abs_cos_theta = abs(wo.z);

        // Sample BSDFs based on the contribution of each BRDF.
        float specular_probability = compute_specular_probability(m_base_tint, specular_tint, m_material.base_roughness, specularity, abs_cos_theta);
        float base_probability = (1.0f - specular_probability);

        return base_probability * BSDFs::Lambert::PDF(wo, wi) +
            specular_probability * BSDFs::GGX::PDF(ggx_alpha, wo, wi);
    }

};

} // NS ShadingModels
} // NS Shading
} // NS OptiXRenderer

#endif // _OPTIXRENDERER_SHADING_MODEL_DEFAULT_SHADING_H_