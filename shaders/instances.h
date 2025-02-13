#ifndef _INSTANCES_H_
#define _INSTANCES_H_

#include "shaders_defines.h"

DeclareStorageBuffer(S_INSTANCE)
{
    mat4 world_space;
    mat4 inv_world_space;
    uvec4 texture_indicies;
    uvec4 globalID;
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
    return instance.globalID.x;
}

#endif // !__cplusplus

#endif // _INSTANCES_H_