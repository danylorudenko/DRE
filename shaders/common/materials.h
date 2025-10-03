#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include "common/shaders_defines.h"

#ifdef __cplusplus
enum MaterialFlags : DRE::U32
{
    MATERIAL_FLAG_NORMAL_TEXTURE                 = 1u << 0,
    MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y        = 1u << 1,
    MATERIAL_FLAG_NORMAL_TBN                     = 1u << 2,
    MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT      = 1u << 3,
    MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES = 1u << 4
};
#else
#define MATERIAL_FLAG_NORMAL_TEXTURE                 (1 << 0)
#define MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y        (1 << 1)
#define MATERIAL_FLAG_NORMAL_TBN                     (1 << 2)
#define MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT      (1 << 3)
#define MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES (1 << 4)
#endif

struct S_MATERIAL
{
    uint4 texture_common_ids;
    uint4 texture_aux_ids;
    uint4 flags;
};

#ifndef __cplusplus

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

int GetAuxTextureID0(S_MATERIAL* material)
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
