// description

#ifndef _SHADER_DEFINES_H_
#define _SHADER_DEFINES_H_

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using vec4 = glm::vec4;
using ivec4 = glm::ivec4;
using uvec4 = glm::uvec4;
using mat4 = glm::mat4;
#endif // __cplusplus

#define PI 3.14159

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

#endif // _SHADER_DEFINES_H_