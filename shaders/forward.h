#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "shaders_defines.h"


#ifndef __cplusplus
// pass outputs
layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;
layout(location = 2) out uint id;
#endif // !__cplusplus
#define FORWARD_PASS_OUTPUT_COUNT 3 // DON'T FORGET


#ifndef __cplusplus
// pass bindings
layout(set = 3, binding = 0) uniform texture2D shadowMap;
#endif // !__cplusplus


#ifdef __cplusplus
struct ForwardUniform {
#else
layout(set = 3, binding = 1, std140) uniform ForwardUniform {
#endif // __cplusplus

    mat4  shadow_VP;
    vec4  shadow_size;

#ifdef __cplusplus
};
#else
} passUniform;
#endif // __cplusplus

#endif // __FORWARD_H__