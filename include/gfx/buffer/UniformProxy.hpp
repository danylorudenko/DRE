#pragma once

#include <foundation\memory\Pointer.hpp>
#include <gfx\buffer\TransientArena.hpp>

namespace VKW
{
class Context;
}

namespace GFX
{

class UniformProxy
    : public NonCopyable
    , public NonMovable
{
public:
    UniformProxy(VKW::Context* context, UniformArena::Allocation const& allocation);

    void WriteMember140(void const* data, std::uint32_t size)
    {
        std::uint32_t padding = size % 16;
        std::uint32_t size140 = size + padding;

        std::memcpy(m_WritePtr, data, size);
        m_WritePtr = DRE::PtrAdd(m_WritePtr, size140);
    }

    template<typename T>
    void WriteMember140(T const& data)
    {
        WriteMember140(&data, sizeof(data));
    }

    ~UniformProxy();

private:
    VKW::Context*                     m_Context;
    typename UniformArena::Allocation m_Allocation;
    void*                             m_WritePtr;
    
};

}


