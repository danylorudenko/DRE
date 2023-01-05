#pragma once

#include <cstdint>

#include <gfx\DeviceChild.hpp>

namespace VKW
{
struct BufferResource;
}

namespace GFX
{

class BufferBase
    : public DeviceChild
{
public:
    explicit BufferBase();
    explicit BufferBase(VKW::Device* device, VKW::BufferResource* bufferResource);

    BufferBase(BufferBase&& rhs);
    BufferBase& operator=(BufferBase&& rhs);

    inline std::uint32_t GetSize() const { return m_Size; }
    inline VKW::BufferResource* GetResource() const { return m_BufferResource; }

    virtual ~BufferBase();

protected:
    VKW::BufferResource*    m_BufferResource;
    std::uint32_t           m_Size;
};

}

