#ifdef __cplusplus
#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

#define vec4 glm::vec4
#define mat4 glm::mat4
#endif


#ifdef __cplusplus
struct GlobalUniform
#else
layout(set = 2, binding = 0, std140) readonly uniform GlobalUniforms
#endif
{
    // uniform buffer layout
    vec4 viewportSize_deltaMS_0;
    // end
#ifdef __cplusplus
};
#else
} g_GlobalUniforms;

// Global uniform values
vec2   GetViewportSize() { return g_GlobalUniforms.viewportSize_deltaMS_0.xy; }
float  GetDeltaTime() { return g_GlobalUniforms.viewportSize_deltaMS_0.z; }
#endif

