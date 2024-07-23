#ifndef __FORWARD_OUTPUT_H__
#define __FORWARD_OUTPUT_H__

#ifndef __cplusplus
// pass outputs
layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;
layout(location = 2) out vec4 id;
#endif // !__cplusplus
#define FORWARD_PASS_OUTPUT_COUNT 3 // DON'T FORGET

#endif // __FORWARD_OUTPUT_H__