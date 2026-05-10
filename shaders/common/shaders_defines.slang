#ifndef _SHADER_DEFINES_SLANG_
#define _SHADER_DEFINES_SLANG_

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using uint      = DRE::U32;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;

using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;

using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
using float4x4  = glm::mat4;

#endif // __cplusplus

/////////////////////////////
// Indirect draw command structures
struct DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

struct DrawIndirectCommand
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

#define PI 3.14159

#endif // _SHADER_DEFINES_SLANG_
