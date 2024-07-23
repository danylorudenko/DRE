#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "shaders_defines.h"

#ifndef __cplusplus
layout(set = 3, binding = 0) uniform texture2D shadowMap;
#endif

//////////////
BEGIN_CONSTANT_BUFFER(ForwardUniform, passUniform, 3, 1)
{
    mat4  shadow_VP;
    vec4  shadow_size;
}
END_CONSTANT_BUFFER(ForwardUniform, passUniform, 3, 1)

#ifndef __cplusplus
layout(set = 3, binding = 2) uniform texture2D causticMap;
#endif

//////////////
BEGIN_CONSTANT_BUFFER(InstanceUniform, instanceUniform, 4, 0)
{
    mat4  model_mat;
    mat4  prev_model_mat;
    uvec4 textureIDs;
    uint  globalID;
}
END_CONSTANT_BUFFER(InstanceUniform, instanceUniform, 4, 0)


#define DiffuseTextureID     instanceUniform.textureIDs[0]
#define NormalTextureID      instanceUniform.textureIDs[1]
#define MetalnessTextureID   instanceUniform.textureIDs[2]
#define RoughnessTextureID   instanceUniform.textureIDs[3]

#ifndef __cplusplus
vec4 GlobalID2Color()
{
    float r = (instanceUniform.globalID >> 0)  & 0x000000FF;
    float g = (instanceUniform.globalID >> 8)  & 0x000000FF;
    float b = (instanceUniform.globalID >> 16) & 0x000000FF;
    //float a = (instanceUniform.globalID >> 24) & 0x000000FF;
    float a = 255; // just for ease of imgui view
    return vec4(r,g,b,a) / 255.0;
}
#endif

#endif // __FORWARD_H__