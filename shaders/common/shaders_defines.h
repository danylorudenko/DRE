// description

#ifndef _SHADER_DEFINES_H_
#define _SHADER_DEFINES_H_

#ifdef __cplusplus

#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

using uint      = std::uint32_t;
using int4      = glm::ivec4;
using uint4     = glm::uvec4;

using float4    = glm::vec4;
using float4x4  = glm::mat4;

#endif // __cplusplus

#define PI 3.14159

// Type_GPURef CPP
#ifdef __cplusplus
    struct GPU_PTR
    {
        std::uint64_t pointer;

        GPU_PTR(std::uint64_t ptr = 0)
            : pointer{ ptr }
        {}

        GPU_PTR& operator=(std::uint64_t ptr)
        {
            pointer = ptr;
            return *this;
        }

        operator std::uint64_t&() { return pointer; }
    };
    #define DeclareStorageBuffer(Type) using Type ## _GPU_PTR = GPU_PTR;

#else // Type_GPU_PTR HLSL
    #define DeclareStorageBuffer(Type) typedef vk::BufferPointer<Type> Type ## _GPU_PTR
#endif // __cplusplus


// Constant Buffers
#ifdef __cplusplus
#define DeclareConstantBuffer(Type, Name, Set, Binding)
#else
#define DeclareConstantBuffer(Type, Name, Set, Binding) [[vk::binding(Binding, Set)]] ConstantBuffer<Type> Name;
#endif // __cplusplus

#endif // _SHADER_DEFINES_H_
