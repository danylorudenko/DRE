#ifndef _INSTANCES_H_
#define _INSTANCES_H_

#include "common/shaders_defines.h"

#ifdef __cplusplus
enum InstanceFlags : uint
{
    TEXTURE             = 1u << 0,
    TEXTURE_INVERT_Y    = 1u << 1,
    TBN                 = 1u << 2
};
#else
#define INSTANCE_FLAG_TEXTURE             (1 << 0)
#define INSTANCE_FLAG_TEXTURE_INVERT_Y    (1 << 1)
#define INSTANCE_FLAG_TBN                 (1 << 2)
#endif

DeclareStorageBuffer(S_INSTANCE)
{
    mat4 world_space;
    mat4 inv_world_space;
    uvec4 texture_indicies;
    uvec4 globalID_instanceFlags;
};

#ifndef __cplusplus

mat4 GetWorldTransform(S_INSTANCE_GPURef instance)
{
    return instance.world_space;
}

mat4 GetInvWorldTransform(S_INSTANCE_GPURef instance)
{
    return instance.inv_world_space;
}

vec3 GetInstanceWorldPos(S_INSTANCE_GPURef instance)
{
    return instance.world_space[3].xyz;
}

uint GetDiffuseTextureID(S_INSTANCE_GPURef instance)
{
    return instance.texture_indicies.x;
}

uint GetNormalTextureID(S_INSTANCE_GPURef instance)
{
    return instance.texture_indicies.y;
}

uint GetMetalnessTextureID(S_INSTANCE_GPURef instance)
{
    return instance.texture_indicies.z;
}

uint GetRoughnessTextureID(S_INSTANCE_GPURef instance)
{
    return instance.texture_indicies.w;
}

uint GetGlobalID(S_INSTANCE_GPURef instance)
{
    return instance.globalID_instanceFlags.x;
}

uint GetInstanceFlags(S_INSTANCE_GPURef instance)
{
    return instance.globalID_instanceFlags.y;
}

#endif // !__cplusplus

#endif // _INSTANCES_H_
