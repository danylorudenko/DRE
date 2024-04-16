// description

#ifndef __SHADER_DEFINES_H__
#define __SHADER_DEFINES_H__

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using vec4 = glm::vec4;
using ivec4 = glm::ivec4;
using uvec4 = glm::uvec4;
using mat4 = glm::mat4;
#endif // __cplusplus

#define PI 3.14159

// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[nonuniformEXT(id)]

// Default samplers
#define GetSamplerNearest() g_GlobalSamplers[0]
#define GetSamplerLinear() g_GlobalSamplers[1]
#define GetSamplerLinearClamp() g_GlobalSamplers[2]
#define GetSamplerAnisotropic() g_GlobalSamplers[3]

#define SampleTexture(textureObj, samplerObj, uv_coords) texture(sampler2D(textureObj, samplerObj), uv_coords)
#define TexelFetchLvl(textureObj, pos, lvl) texelFetch(sampler2D(textureObj, GetSamplerNearest(), pos, lvl)
#define TexelFetch(textureObj, pos) texelFetch(sampler2D(textureObj, GetSamplerNearest()), pos, 0)

// Type_GPURef CPP
#ifdef __cplusplus
    struct GPUPointer
    {
        std::uint64_t pointer;
        std::uint64_t ___pad;

        GPUPointer(std::uint64_t ptr = 0)
            : pointer{ ptr }
            , ___pad{ 0 }
        {}

        GPUPointer& operator=(std::uint64_t ptr)
        {
            pointer = ptr;
            return *this;
        }

        operator std::uint64_t&() { return pointer; }
    };
    #define DeclareStorageBuffer(Type) using Type ## _GPURef = GPUPointer; struct Type

#else // Type_GPURef GLSL
    #define DeclareStorageBuffer(Type) layout(buffer_reference, std430, buffer_reference_align = 16) buffer Type ## _GPURef
#endif // __cplusplus

#endif // __SHADER_DEFINES_H__