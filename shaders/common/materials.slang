#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include "common/shaders_defines.slang"

#define MATERIAL_FLAG_USE_PBR                           (1 << 0)
#define MATERIAL_FLAG_USE_SPEC_GLOSS                    (1 << 1)
#define MATERIAL_FLAG_NORMAL_TEXTURE                    (1 << 2)
#define MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y           (1 << 3)
#define MATERIAL_FLAG_NORMAL_TBN                        (1 << 4)
#define MATERIAL_FLAG_METALLIC_ROUGNESS_COMBINED        (1 << 5)
#define MATERIAL_FLAG_ALPHA_MASKED                      (1 << 6)
#define MATERIAL_FLAG_BUMP_TEXTURE                      (1 << 7)

#ifdef __cplusplus
using MaterialFlags = DRE::U32;
#endif // __cplusplus



struct S_MATERIAL
{
    uint4 texture_ids0;
    uint4 texture_ids1;
    uint4 texture_ids2;
    uint4 flags;
};



#ifndef __cplusplus
#include "common/shaders_common.slang"

struct S_MATERIAL_PROPERTIES
{
    float3 diffuse = float3(0,0,0);
    float3 normal = float3(0,0,1);
    float height = 0;
    float metalness = 1;
    float roughness = 1;
    float opacity = 1;
};

interface IMaterialTextureSampler
{
    float4 SampleTextureNearest(uint textureID, float2 UV);
    float4 SampleTextureLinear(uint textureID, float2 UV);
    float4 SampleTextureAnisotropic(uint textureID, float2 UV);
};

struct MaterialTextureSampler : IMaterialTextureSampler
{
    float4 SampleTextureNearest(uint textureID, float2 UV)
    {
        return SampleGlobalTextureNearest(textureID, UV);
    }
    float4 SampleTextureLinear(uint textureID, float2 UV)
    {
        return SampleGlobalTextureLinear(textureID, UV);
    }
    float4 SampleTextureAnisotropic(uint textureID, float2 UV)
    {
        return SampleGlobalTextureAnisotropic(textureID, UV);
    }
};

struct ComputeMaterialTextureSampler : IMaterialTextureSampler
{
    float mipLevel;

    float4 SampleTextureNearest(uint textureID, float2 UV)
    {
        return SampleGlobalTextureNearest(textureID, UV, mipLevel);
    }
    float4 SampleTextureLinear(uint textureID, float2 UV)
    {
        return SampleGlobalTextureLinear(textureID, UV, mipLevel);
    }
    float4 SampleTextureAnisotropic(uint textureID, float2 UV)
    {
        return SampleGlobalTextureAnisotropic(textureID, UV, mipLevel);
    }
};


S_MATERIAL_PROPERTIES ReadMaterialProperties<TSampler : IMaterialTextureSampler>(S_MATERIAL* material, float2 UV, float3x3 TBN, TSampler sampler)
{
    S_MATERIAL_PROPERTIES result = {};

    if (IsMaterialPBR(material))
    {
        ReadMaterialPropertiesPBR(material, UV, TBN, result, sampler);
    }
    else if (IsMaterialSpecGloss(material))
    {
        ReadMaterialPropertiesSpecGloss(material, UV, TBN, result, sampler);
    }
    else
    {
        ReadMaterialPropertiesDiffuseOnly(material, UV, TBN, result, sampler);
    }

    return result;
}

void ReadMaterialPropertiesNormalsHELPER<TSampler : IMaterialTextureSampler>(S_MATERIAL* material, in float2 UV, in float3x3 TBN, inout S_MATERIAL_PROPERTIES result, TSampler sampler)
{
    if (IsMaterialNormalTextureEnabled(material))
    {
        result.normal = sampler.SampleTextureAnisotropic(GetNormalTextureID(material), UV).rgb;
        result.normal = result.normal * 2.0f - 1.0f;

        if (IsMaterialNormalTextureInvertY(material))
        {
            result.normal.y = -result.normal.y;
        }

        result.normal = normalize(mul(result.normal, TBN));
    }
    else if (IsMaterialBumpTextureEnabled(material))
    {
        result.height = sampler.SampleTextureAnisotropic(GetBumpTextureID(material), UV).r;
        float2 dims;
        GetGlobalTextureObject(GetBumpTextureID(material)).GetDimensions(dims.x, dims.y);
        float2 pixelSize = rcp(dims);

        float rightHeight = sampler.SampleTextureAnisotropic(GetBumpTextureID(material), UV + float2(pixelSize.x, 0)).r - result.height;
        float rightDistance = sqrt(1 - rightHeight * rightHeight);
        float3 tanVector = float3(rightDistance, 0, rightHeight);

        float upperHeight = sampler.SampleTextureAnisotropic(GetBumpTextureID(material), UV + float2(0, pixelSize.y)).r - result.height;
        float upperDistance = sqrt(1 - upperHeight * upperHeight);
        float3 bitanVector = float3(0, upperDistance, upperHeight);

        float3 tangentNormal = cross(tanVector, bitanVector);
        result.normal = normalize(mul(tangentNormal, TBN));
    }
    else if (IsMaterialNormalTBNEnabled(material))
    {
        result.normal = TBN[2];
    }
}

void ReadMaterialPropertiesPBR<TSampler : IMaterialTextureSampler>(S_MATERIAL* material, in float2 UV, in float3x3 TBN, inout S_MATERIAL_PROPERTIES result, TSampler sampler)
{
    result.diffuse = sampler.SampleTextureAnisotropic(GetDiffuseTextureID(material), UV).rgb;
    if (IsMaterialAlphaMasked(material))
    {
        result.opacity = sampler.SampleTextureAnisotropic(GetOpacityTextureID(material), UV).r;
    }

    ReadMaterialPropertiesNormalsHELPER(material, UV, TBN, result, sampler);

    if (IsMaterialMetallicRoughnessCombined(material))
    {
        float2 metalness_roughness = sampler.SampleTextureAnisotropic(GetMetalnessTextureID(material), UV).bg;
        result.metalness = metalness_roughness.x;
        result.roughness = metalness_roughness.y;
    }
    else
    {
        result.metalness = sampler.SampleTextureAnisotropic(GetMetalnessTextureID(material), UV).r;
        result.roughness = sampler.SampleTextureAnisotropic(GetRoughnessTextureID(material), UV).r;
    }
}

void ReadMaterialPropertiesSpecGloss<TSampler : IMaterialTextureSampler>(S_MATERIAL* material, in float2 UV, in float3x3 TBN, inout S_MATERIAL_PROPERTIES result, TSampler sampler)
{
    result.diffuse = sampler.SampleTextureAnisotropic(GetDiffuseTextureID(material), UV).rgb;
    if (IsMaterialAlphaMasked(material))
    {
        result.opacity = sampler.SampleTextureAnisotropic(GetOpacityTextureID(material), UV).r;
    }

    ReadMaterialPropertiesNormalsHELPER(material, UV, TBN, result, sampler);

    float3 glossiness = sampler.SampleTextureAnisotropic(GetGlossinessTextureID(material), UV).rgb;
    float glossinessAverage = max(glossiness.r, max(glossiness.g, glossiness.b));
    result.roughness = 1.0f - glossinessAverage;
    result.roughness = result.roughness * result.roughness; // perceptual mapping

    float3 specular = sampler.SampleTextureAnisotropic(GetSpecularTextureID(material), UV).rgb;
    specular = specular * specular; // perceptual mapping
    result.metalness = dot(specular, float3(0.2126, 0.7152, 0.0722));
}

void ReadMaterialPropertiesDiffuseOnly<TSampler : IMaterialTextureSampler>(S_MATERIAL* material, in float2 UV, in float3x3 TBN, inout S_MATERIAL_PROPERTIES result, TSampler sampler)
{
    result.diffuse = sampler.SampleTextureAnisotropic(GetDiffuseTextureID(material), UV).rgb;
    if (IsMaterialAlphaMasked(material))
    {
        result.opacity = sampler.SampleTextureAnisotropic(GetOpacityTextureID(material), UV).r;
    }

    ReadMaterialPropertiesNormalsHELPER(material, UV, TBN, result, sampler);
}

uint GetDiffuseTextureID(S_MATERIAL* material)
{
    return material->texture_ids0.x;
}

uint GetNormalTextureID(S_MATERIAL* material)
{
    return material->texture_ids0.y;
}

uint GetMetalnessTextureID(S_MATERIAL* material)
{
    return material->texture_ids0.z;
}

uint GetRoughnessTextureID(S_MATERIAL* material)
{
    return material->texture_ids0.w;
}

uint GetOcclusionTextureID(S_MATERIAL* material)
{
    return material->texture_ids1.x;
}

uint GetBentNormalTextureID(S_MATERIAL* material)
{
    return material->texture_ids1.y;
}

uint GetOpacityTextureID(S_MATERIAL* material)
{
    return material->texture_ids1.z;
}

uint GetGlossinessTextureID(S_MATERIAL* material)
{
    return material->texture_ids1.w;
}

uint GetSpecularTextureID(S_MATERIAL* material)
{
    return material->texture_ids2.x;
}

uint GetBumpTextureID(S_MATERIAL* material)
{
    return material->texture_ids2.y;
}

uint GetMaterialFlags(S_MATERIAL* material)
{
    return asuint(material->flags.x);
}

bool IsMaterialPBR(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_USE_PBR) != 0;
}

bool IsMaterialSpecGloss(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_USE_SPEC_GLOSS) != 0;
}

bool IsMaterialNormalTextureEnabled(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TEXTURE) != 0;
}

bool IsMaterialNormalTextureInvertY(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0;
}

bool IsMaterialNormalTBNEnabled(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TBN) != 0;
}

bool IsMaterialBumpTextureEnabled(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_BUMP_TEXTURE) != 0;
}

bool IsMaterialMetallicRoughnessCombined(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_METALLIC_ROUGNESS_COMBINED) != 0;
}

bool IsMaterialAlphaMasked(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_ALPHA_MASKED) != 0;
}

#endif // !__cplusplus

#endif // _MATERIALS_H_
