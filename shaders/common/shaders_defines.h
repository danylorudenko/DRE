// description

#ifndef _SHADER_DEFINES_H_
#define _SHADER_DEFINES_H_

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using uint  = std::uint32_t;
using vec4  = glm::vec4;
using ivec4 = glm::ivec4;
using uvec4 = glm::uvec4;
using mat4  = glm::mat4;

#else // HLSL

typedef uint2    uvec2;
typedef uint3    uvec3;
typedef uint4    uvec4;
typedef int2     ivec2;
typedef int3     ivec3;
typedef int4     ivec4;
typedef float2   vec2;
typedef float3   vec3;
typedef float4   vec4;
typedef float4x4 mat4;

#endif // __cplusplus

#define PI 3.14159

// Type_GPURef CPP
#ifdef __cplusplus
    struct GPUPointer
    {
        std::uint64_t pointer;

        GPUPointer(std::uint64_t ptr = 0)
            : pointer{ ptr }
        {}

        GPUPointer& operator=(std::uint64_t ptr)
        {
            pointer = ptr;
            return *this;
        }

        operator std::uint64_t&() { return pointer; }
    };
    #define DeclareStorageBuffer(Type) using Type ## _GPURef = GPUPointer; struct Type

#else // Type_GPURef HLSL
    #define DeclareStorageBuffer(Type) [[vk::buffer_reference]] [[vk::buffer_reference_align(16)]] struct Type ## _GPURef
#endif // __cplusplus


// Constant Buffers
#ifdef __cplusplus
#define BEGIN_CONSTANT_BUFFER(Type, Name, Set, Binding) struct Type
#define END_CONSTANT_BUFFER(Type, Name, Set, Binding) ;
#else
#define BEGIN_CONSTANT_BUFFER(Type, Name, Set, Binding) struct Type
#define END_CONSTANT_BUFFER(Type, Name, Set, Binding) ; [[vk::binding(Binding, Set)]] ConstantBuffer<Type> Name;
#endif // __cplusplus

#endif // _SHADER_DEFINES_H_
