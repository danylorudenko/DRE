#ifndef _TRANSFORMS_H_
#define _TRANSFORMS_H_

#include "shaders_defines.h"

DeclareStorageBuffer(S_TRANSFORM)
{
    mat4 world_space;
    //mat4 inv_world_space;
};

#ifndef __cplusplus

vec3 GetWorldPos(S_TRANSFORM_GPURef transform)
{
    return transform.world_space[3].xyz;
}

#endif // !__cplusplus

#endif // _TRANSFORMS_H_