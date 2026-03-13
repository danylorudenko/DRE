#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include "common/shaders_defines.h"

#define MATERIAL_FLAG_NORMAL_TEXTURE                    (1 << 0)
#define MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y           (1 << 1)
#define MATERIAL_FLAG_NORMAL_TBN                        (1 << 2)
#define MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT         (1 << 3)
#define MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES    (1 << 4)
#define MATERIAL_FLAG_ALPHA_MASKED                      (1 << 5)

#ifdef __cplusplus
using MaterialFlags = DRE::U32;
#endif // __cplusplus



struct S_MATERIAL
{
    uint4 texture_common_ids;
    uint4 texture_aux_ids;
    uint4 flags;
};



#ifndef __cplusplus
#include "common/shaders_common.h"

struct S_MATERIAL_PROPERTIES
{
    float3 diffuse = float3(0,0,0);
    float3 normal = float3(0,0,1);
    float metalness = 0;
    float roughness = 0;
    float opacity = 1;
};

S_MATERIAL_PROPERTIES ReadMaterialProperties(S_MATERIAL* material, float2 UV, float3x3 TBN)
{
    S_MATERIAL_PROPERTIES result = {};

    uint materialFlags = GetMaterialFlags(material);

    result.diffuse    = SampleGlobalTextureAnisotropic(GetDiffuseTextureID(material), UV).rgb;
    result.normal     = SampleGlobalTextureAnisotropic(GetNormalTextureID(material), UV).rgb;

    if ((materialFlags & MATERIAL_FLAG_ALPHA_MASKED) != 0)
    {
        result.opacity = SampleGlobalTextureAnisotropic(GetOpacityTextureID(material), UV).a;
    }

    if ((materialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT) != 0)
    {
        result.metalness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(material), UV).r;
        result.roughness = SampleGlobalTextureAnisotropic(GetRoughnessTextureID(material), UV).r;
    }
    if ((materialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES) != 0)
    {
        float2 metalness_roughness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(material), UV).bg;
        result.metalness = metalness_roughness.x;
        result.roughness = metalness_roughness.y;
    }

    if ((materialFlags & MATERIAL_FLAG_NORMAL_TEXTURE) != 0)
    {
        result.normal = normalize(mul(TBN, (result.normal * 2.0 - 1.0)));
    }

    if ((materialFlags & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0)
    {
        result.normal = float3(result.normal.r, 1 - result.normal.g, result.normal.b);
        result.normal = normalize(mul(TBN, (result.normal * 2.0 - 1.0)));
    }

    if ((materialFlags & MATERIAL_FLAG_NORMAL_TBN) != 0)
    {
        result.normal = TBN[2];
    }

    return result;
}

int GetDiffuseTextureID(S_MATERIAL* material)
{
    return material->texture_common_ids.x;
}

int GetNormalTextureID(S_MATERIAL* material)
{
    return material->texture_common_ids.y;
}

int GetMetalnessTextureID(S_MATERIAL* material)
{
    return material->texture_common_ids.z;
}

int GetRoughnessTextureID(S_MATERIAL* material)
{
    return material->texture_common_ids.w;
}

int GetOcclusionTextureID(S_MATERIAL* material)
{
    return material->texture_aux_ids.x;
}

int GetBentNormalTextureID(S_MATERIAL* material)
{
    return material->texture_aux_ids.y;
}

int GetOpacityTextureID(S_MATERIAL* material)
{
    return material->texture_aux_ids.z;
}

int GetAuxTextureID1(S_MATERIAL* material)
{
    return material->texture_aux_ids.w;
}

uint GetMaterialFlags(S_MATERIAL* material)
{
    return asuint(material->flags.x);
}

bool IsNormalTextureEnabled(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TEXTURE) != 0;
}

bool IsNormalTextureInvertY(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0;
}

bool IsNormalTBNEnabled(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_NORMAL_TBN) != 0;
}

bool IsMaterialTexturesDefault(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT) != 0;
}

bool IsMaterialTexturesGLTFSpheres(S_MATERIAL* material)
{
    return (GetMaterialFlags(material) & MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES) != 0;
}

#endif // !__cplusplus

#endif // _MATERIALS_H_
