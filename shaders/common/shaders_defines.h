// description

#ifndef _SHADER_DEFINES_H_
#define _SHADER_DEFINES_H_

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using uint      = std::uint32_t;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;

using int       = std::int32_t;
using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;

using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
using float4x4  = glm::mat4;

#endif // __cplusplus

#define PI 3.14159

#endif // _SHADER_DEFINES_H_
