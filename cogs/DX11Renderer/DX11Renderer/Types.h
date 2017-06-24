// DirectX 11 renderer host types.
//-------------------------------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
//-------------------------------------------------------------------------------------------------

#ifndef _DX11RENDERER_RENDERER_TYPES_H_
#define _DX11RENDERER_RENDERER_TYPES_H_

#include <Cogwheel/Math/half.h>

#include <DX11Renderer/UniqueResourcePtr.h>

//-------------------------------------------------------------------------------------------------
// Forward declarations.
//-------------------------------------------------------------------------------------------------
struct ID3D10Blob;
using ID3DBlob = ID3D10Blob;
struct ID3D11Buffer;
struct ID3D11ComputeShader;
struct ID3D11Device1;
struct ID3D11DeviceContext1;
struct ID3D11InputLayout;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct ID3D11VertexShader;
struct ID3D11UnorderedAccessView;

namespace DX11Renderer {

//-------------------------------------------------------------------------------------------------
// Alias unique resource pointers
//-------------------------------------------------------------------------------------------------

using UID3DBlob = DX11Renderer::UniqueResourcePtr<ID3DBlob>;
using UID3D11Buffer = DX11Renderer::UniqueResourcePtr<ID3D11Buffer>;
using UID3D11ComputeShader = DX11Renderer::UniqueResourcePtr<ID3D11ComputeShader>;
using UID3D11Device1 = DX11Renderer::UniqueResourcePtr<ID3D11Device1>;
using UID3D11DeviceContext1 = DX11Renderer::UniqueResourcePtr<ID3D11DeviceContext1>;
using UID3D11InputLayout = DX11Renderer::UniqueResourcePtr<ID3D11InputLayout>;
using UID3D11PixelShader = DX11Renderer::UniqueResourcePtr<ID3D11PixelShader>;
using UID3D11SamplerState = DX11Renderer::UniqueResourcePtr<ID3D11SamplerState>;
using UID3D11ShaderResourceView = DX11Renderer::UniqueResourcePtr<ID3D11ShaderResourceView>;
using UID3D11Texture2D = DX11Renderer::UniqueResourcePtr<ID3D11Texture2D>;
using UID3D11VertexShader = DX11Renderer::UniqueResourcePtr<ID3D11VertexShader>;
using UID3D11UnorderedAccessView = DX11Renderer::UniqueResourcePtr<ID3D11UnorderedAccessView>;


//-------------------------------------------------------------------------------------------------
// Storage.
//-------------------------------------------------------------------------------------------------

#define CONSTANT_ELEMENT_SIZE 16
#define CONSTANT_BUFFER_ALIGNMENT 256

struct float2 {
    float x; float y;
};

struct float3 {
    float x; float y; float z;
};

struct float4 {
    float x; float y; float z;  float w;
};

struct AABB {
    float3 min;
    float3 max;
};

struct R11G11B10_Float {
    unsigned int raw;

    R11G11B10_Float() : raw(0) {}

    R11G11B10_Float(float r, float g, float b) {
        // Pack RGB into R11G11B10. All three channels have a 5 bit exponent and no sign bit.
        // This fits perfectly with the half format, that also have a 5 bit exponent.
        // The solution therefore is to convert the float to half and then using bitmasks 
        // to extract the 5 bit exponent and the first 5/6 bits of the mantissa.
        // The memory layout is confusingly [B10, G11, R11]. Go figure.
        unsigned int blue = (half_float::half(b).raw() & 0x7FE0) << 17;
        unsigned int green = (half_float::half(g).raw() & 0x7FF0) << 7;
        unsigned int red = (half_float::half(r).raw() & 0x7FF0) >> 4;
        raw = red | green | blue;
    }
};

//-------------------------------------------------------------------------------------------------
// Model structs.
//-------------------------------------------------------------------------------------------------

struct Dx11Material {
    float3 tint;
    unsigned int tint_texture_index;
    float roughness;
    float specularity;
    float metallic;
    float coverage;
    unsigned int coverage_texture_index;
    float3 __padding;
};

struct Dx11Image {
    UID3D11ShaderResourceView srv;
};

struct Dx11Texture {
    Dx11Image* image;
    UID3D11SamplerState sampler;
};

struct Dx11Mesh {
    unsigned int index_count;
    unsigned int vertex_count;
    ID3D11Buffer* indices;

    ID3D11Buffer* buffers[3]; // [positions, normals, texcoords]
    int buffer_count;

    AABB bounds;

    ID3D11Buffer* positions() { return buffers[0]; }
    ID3D11Buffer** positions_address() { return buffers; }
    ID3D11Buffer* normals() { return buffers[1]; }
    ID3D11Buffer** normals_address() { return buffers + 1; }
    ID3D11Buffer* texcoords() { return buffers[2]; }
    ID3D11Buffer** texcoords_address() { return buffers + 2; }
};

struct Dx11Model {
    struct Properties {
        static const unsigned int None = 0u;
        static const unsigned int Cutout = 1u << 0u;
        static const unsigned int Transparent = 1u << 1u;
        static const unsigned int Destroyed = 1u << 31u;
    };

    unsigned int model_ID;
    unsigned int mesh_ID;
    unsigned int transform_ID;
    unsigned int material_ID;
    unsigned int properties; // NOTE If I really really really wanted to keep this 16 byte aligned (which is nice), then I could store properties in upper 8 bits of the IDs.

    bool is_opaque() { return (properties & (Properties::Cutout | Properties::Transparent)) == 0; }
    bool is_cutout() { return (properties & Properties::Cutout) == Properties::Cutout; }
    bool is_transparent() { return (properties & Properties::Transparent) == Properties::Transparent; }
    bool is_destroyed() { return (properties & Properties::Destroyed) == Properties::Destroyed; }
};

//-------------------------------------------------------------------------------------------------
// Light source structs.
//-------------------------------------------------------------------------------------------------

struct Dx11SphereLight {
    float3 power;
    float3 position;
    float radius;
};

struct Dx11DirectionalLight {
    float3 radiance;
    float3 direction;
    float __padding;
};

struct Dx11Light{
    enum Flags {
        None = 0u,
        Sphere = 1u,
        Directional = 2u,
        TypeMask = 3u
    };

    float flags;

    union {
        Dx11SphereLight sphere;
        Dx11DirectionalLight directional;
    };

    unsigned int get_type() const { return (Flags)(unsigned int)flags; }
    bool is_type(Flags light_type) { return get_type() == light_type; }
};

} // NS DX11Renderer

#endif // _DX11RENDERER_RENDERER_TYPES_H_