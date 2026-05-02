#ifndef _INSTANCES_H_
#define _INSTANCES_H_

#include "common/shaders_defines.slang"
#include "common/materials.slang"

#ifdef __cplusplus
enum InstanceFlags : DRE::U32
{
    INSTANCE_FLAG_DUMMY                 = 1u << 0
};
#else
#define INSTANCE_FLAG_DUMMY                 (1 << 0)
#endif

struct S_INSTANCE
{
    float4x4    world_space;
    float4x4    inv_world_space;
    uint4       globalID_instanceFlags;
    S_MATERIAL* material;
};

#ifndef __cplusplus

float4x4 GetWorldTransform(S_INSTANCE* instance)
{
    return instance.world_space;
}

float4x4 GetInvWorldTransform(S_INSTANCE* instance)
{
    return instance.inv_world_space;
}

float3 GetInstanceWorldPos(S_INSTANCE* instance)
{
    return instance.world_space[3].xyz;
}

uint GetGlobalID(S_INSTANCE* instance)
{
    return instance.globalID_instanceFlags.x;
}

float4 GlobalID2Color()
{
    S_INSTANCE* Instance = GetInstance();

    float r = (GetGlobalID(Instance) & 0xFF000000) >> 24;
    float g = (GetGlobalID(Instance) & 0x00FF0000) >> 16;
    float b = (GetGlobalID(Instance) & 0x0000FF00) >> 8;
    float a = (GetGlobalID(Instance) & 0x000000FF) >> 0;
    return float4(r,g,b,a) / 255.0f;
}

uint GetInstanceFlags(S_INSTANCE* instance)
{
    return instance.globalID_instanceFlags.y;
}

S_MATERIAL* GetMaterial(S_INSTANCE* instance)
{
    return instance.material;
}

#endif // !__cplusplus

#endif // _INSTANCES_H_
