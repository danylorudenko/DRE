#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "common/shaders_defines.h"
#include "common/global_uniform.h"

//////////////
#ifndef __cplusplus
uint GetInstanceID() { return globalPushConstant.value_int1; }
S_INSTANCE_GPURef GetInstance() { return GetInstance(GetInstanceID()); }
#endif

//////////////
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
// see InstanceDataManager
//BEGIN_CONSTANT_BUFFER(InstanceUniform, instanceUniform, 4, 0)
//{
//    mat4  model_mat;
//    mat4  prev_model_mat;
//    uvec4 textureIDs;
//    uint  globalID;
//}
//END_CONSTANT_BUFFER(InstanceUniform, instanceUniform, 4, 0)


//#define DiffuseTextureID     instanceUniform.textureIDs[0]
//#define NormalTextureID      instanceUniform.textureIDs[1]
//#define MetalnessTextureID   instanceUniform.textureIDs[2]
//#define RoughnessTextureID   instanceUniform.textureIDs[3]

#ifndef __cplusplus
vec4 GlobalID2Color()
{
    float r = (GetInstanceID() & 0xFF000000) >> 24;
    float g = (GetInstanceID() & 0x00FF0000) >> 16;
    float b = (GetInstanceID() & 0x0000FF00) >> 8;
    float a = (GetInstanceID() & 0x000000FF) >> 0;
    return vec4(r,g,b,a) / 255.0;
}
#endif

#endif // __FORWARD_H__