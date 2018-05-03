// Bloom shaders.
// ------------------------------------------------------------------------------------------------
// Copyright (C) 2018, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#include "Utils.hlsl"

namespace CameraEffects {

Texture2D image : register(t0);
SamplerState image_sampler : register(s0);
RWTexture2D<float4> output_image : register(u0);

// ------------------------------------------------------------------------------------------------
// Gaussian bloom.
// ------------------------------------------------------------------------------------------------

[numthreads(32, 32, 1)]
void gaussian_horizontal_filter(uint3 global_thread_ID : SV_DispatchThreadID) {
    float width, height;
    output_image.GetDimensions(width, height);
    
    float total_weight = 0.0; // Precompute
    float3 sum = 0.0;
    for (int x = -bloom_bandwidth; x <= bloom_bandwidth; ++x) {
        float weight = exp(-(x * x) / bloom_2x_variance);

        int2 index = global_thread_ID.xy + int2(x, 0);
        index.x = clamp(index.x, 0, width - 1);

        sum += max(0.0, image[index].rgb - bloom_threshold) * weight;
        total_weight += weight;
    }

    output_image[global_thread_ID.xy] = float4(sum / total_weight, 1.0);
}

[numthreads(32, 32, 1)]
void gaussian_vertical_filter(uint3 global_thread_ID : SV_DispatchThreadID) {
    float width, height;
    output_image.GetDimensions(width, height);

    float total_weight = 0.0; // Precompute
    float3 sum = 0.0;
    for (int y = -bloom_bandwidth; y <= bloom_bandwidth; ++y) {
        float weight = exp(-(y * y) / bloom_2x_variance);

        int2 index = global_thread_ID.xy + int2(0, y);
        index.y = clamp(index.y, 0, height - 1);

        sum += image[index].rgb * weight;
        total_weight += weight;
    }

    output_image[global_thread_ID.xy] = float4(sum / total_weight, 1.0);
}

// ------------------------------------------------------------------------------------------------
// Dual kawase filtering
// https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-26-50/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf
// ------------------------------------------------------------------------------------------------

[numthreads(32, 32, 1)]
void extract_high_intensity(uint3 global_thread_ID : SV_DispatchThreadID) {
    float4 pixel = image[global_thread_ID.xy];
    output_image[global_thread_ID.xy] = float4(max(0.0, pixel.rgb - bloom_threshold), pixel.a);
}

[numthreads(32, 32, 1)]
void dual_kawase_downsample(uint3 global_thread_ID : SV_DispatchThreadID) {
    float width, height;
    output_image.GetDimensions(width, height);

    const float2 half_pixel_width = 0.5 * rcp(float2(width, height));
    float2 uv = global_thread_ID.xy * rcp(float2(width, height)) + half_pixel_width;

    float4 sum = image.SampleLevel(image_sampler, uv, 0) * 4.0;
    sum += image.SampleLevel(image_sampler, uv + float2( half_pixel_width.x,  half_pixel_width.y), 0);
    sum += image.SampleLevel(image_sampler, uv + float2( half_pixel_width.x, -half_pixel_width.y), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(-half_pixel_width.x,  half_pixel_width.y), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(-half_pixel_width.x, -half_pixel_width.y), 0);

    output_image[global_thread_ID.xy] = sum / 8.0;
}

[numthreads(32, 32, 1)]
void dual_kawase_upsample(uint3 global_thread_ID : SV_DispatchThreadID) {
    float width, height;
    output_image.GetDimensions(width, height);

    const float2 half_pixel_width = 0.5 * rcp(float2(width, height));
    const float2 pixel_width = half_pixel_width * 2.0;;

    float2 uv = global_thread_ID.xy * rcp(float2(width, height)) + half_pixel_width;

    float4 sum = image.SampleLevel(image_sampler, uv + float2(-half_pixel_width.x * 2.0, 0.0), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(-half_pixel_width.x, half_pixel_width.y), 0) * 2.0;
    sum += image.SampleLevel(image_sampler, uv + float2(0.0, half_pixel_width.y * 2.0), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(half_pixel_width.x, half_pixel_width.y), 0) * 2.0;
    sum += image.SampleLevel(image_sampler, uv + float2(half_pixel_width.x * 2.0, 0.0), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(half_pixel_width.x, -half_pixel_width.y), 0) * 2.0;
    sum += image.SampleLevel(image_sampler, uv + float2(0.0, -half_pixel_width.y * 2.0), 0);
    sum += image.SampleLevel(image_sampler, uv + float2(-half_pixel_width.x, -half_pixel_width.y), 0) * 2.0;

    output_image[global_thread_ID.xy] = sum / 12.0;
}

} // NS CameraEffects