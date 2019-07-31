// OptiX renderer public POD types.
// ------------------------------------------------------------------------------------------------
// Copyright (C) Bifrost. See AUTHORS.txt for authors.
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_PUBLIC_TYPES_H_
#define _OPTIXRENDERER_PUBLIC_TYPES_H_

#include <Bifrost/Core/Bitmask.h>

namespace OptiXRenderer {

// ------------------------------------------------------------------------------------------------
// Different rendering backends.
// ------------------------------------------------------------------------------------------------
enum class Backend {
    None,
    PathTracing,
    AIDenoisedPathTracing,
    AlbedoVisualization,
    NormalVisualization,
};

// ------------------------------------------------------------------------------------------------
// AI denoiser flags.
// ------------------------------------------------------------------------------------------------
enum class AIDenoiserFlag : unsigned char {
    None = 0,
    Albedo = 1 << 0,
    Normals = 1 << 1,
    GammaCorrect = 1 << 2,
    LogarithmicFeedback = 1 << 3,
    // Mutually exclusive debug flags
    VisualizeNoise = 1 << 4,
    VisualizeAlbedo = 1 << 5,
    VisualizeNormals = 1 << 6,
    Default = Albedo | Normals
};
typedef Bifrost::Core::Bitmask<AIDenoiserFlag> AIDenoiserFlags;

} // NS OptiXRenderer

#endif // _OPTIXRENDERER_PUBLIC_TYPES_H_