// OptiXRenderer presampled environment map.
// ------------------------------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. 
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#include <OptiXRenderer/PresampledEnvironmentMap.h>

#include <Cogwheel/Assets/InfiniteAreaLight.h>
#include <Cogwheel/Math/RNG.h>

using namespace Cogwheel;
using namespace optix;

namespace OptiXRenderer {

PresampledEnvironmentMap::PresampledEnvironmentMap(Context& context, Assets::Textures::UID environment_map_ID, 
                                                   TextureSampler* texture_cache, int sample_count) {
    
    Assets::TextureND environment_map = environment_map_ID;
    int width = environment_map.get_image().get_width();
    int height = environment_map.get_image().get_height();
    Assets::InfiniteAreaLight light = Assets::InfiniteAreaLight(environment_map_ID);

    { // Per pixel PDF sampler.
        optix::Buffer per_pixel_PDF_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, width, height);
        float* per_pixel_PDF_data = static_cast<float*>(per_pixel_PDF_buffer->map());
        Cogwheel::Assets::InfiniteAreaLight::compute_PDF(environment_map_ID, per_pixel_PDF_data);

        float PDF_normalization_term = 1.0f / (float(light.image_integral()) * 2.0f * PIf * PIf);
        #pragma omp parallel for schedule(dynamic, 16)
        for (int i = 0; i < int(width * height); ++i)
            per_pixel_PDF_data[i] *= PDF_normalization_term;
        per_pixel_PDF_buffer->unmap();

        m_per_pixel_PDF = context->createTextureSampler();
        m_per_pixel_PDF->setWrapMode(0, RT_WRAP_CLAMP_TO_EDGE);
        m_per_pixel_PDF->setWrapMode(1, RT_WRAP_CLAMP_TO_EDGE);
        m_per_pixel_PDF->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
        m_per_pixel_PDF->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE); // Data is already in floating point format, so no need to normalize it.
        m_per_pixel_PDF->setMaxAnisotropy(0.0f);
        m_per_pixel_PDF->setMipLevelCount(1u);
        m_per_pixel_PDF->setFilteringModes(RT_FILTER_NEAREST, RT_FILTER_NEAREST, RT_FILTER_NONE);
        m_per_pixel_PDF->setArraySize(1u);
        m_per_pixel_PDF->setBuffer(0u, 0u, per_pixel_PDF_buffer);
        OPTIX_VALIDATE(m_per_pixel_PDF);
    }

    { // Draw light samples.
        sample_count = sample_count <= 0 ? 8192 : sample_count; // TODO next pow and base on texture size. Minimally 1024 samples.

        m_samples = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER, sample_count);
        m_samples->setElementSize(sizeof(LightSample));
        LightSample* m_samples_data = static_cast<LightSample*>(m_samples->map());
        #pragma omp parallel for schedule(dynamic, 16)
        for (int i = 0; i < sample_count; ++i) {
            Assets::LightSample sample = light.sample(Math::RNG::sample02(i));
            m_samples_data[i].radiance = { sample.radiance.r, sample.radiance.g, sample.radiance.b };
            m_samples_data[i].PDF = sample.PDF;
            m_samples_data[i].direction_to_light = { sample.direction_to_light.x, sample.direction_to_light.y, sample.direction_to_light.z };
            m_samples_data[i].distance = sample.distance;
        }
        m_samples->unmap();
        OPTIX_VALIDATE(m_samples);

        // TODO Renormalize the samples? Or just draw enough that the PDF is close to one?
    }

    // Fill the presampled light.
    m_light.samples_ID = m_samples->getId();
    m_light.per_pixel_PDF_ID = m_per_pixel_PDF->getId();
    m_light.sample_count = sample_count;
    m_light.environment_map_ID = texture_cache[environment_map_ID]->getId();
}

PresampledEnvironmentMap::~PresampledEnvironmentMap() {
    if (m_samples) m_samples->destroy();
    if (m_per_pixel_PDF) m_per_pixel_PDF->destroy();
}

} // NS OptiXRenderer
