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

#endif // __FORWARD_H__