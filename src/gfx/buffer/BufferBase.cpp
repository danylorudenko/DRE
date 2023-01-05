#include <gfx\buffer\BufferBase.hpp>

#include <foundation\Common.hpp>
#include <vk_interface\Device.hpp>

namespace GFX
{

BufferBase::BufferBase()
    : DeviceChild{ nullptr }
    , m_BufferResource{ nullptr }
    , m_Size{ 0 }
{}

BufferBase::BufferBase(VKW::Device* device, VKW::BufferResource* bufferResource)
    : DeviceChild{ device }
    , m_BufferResource{ bufferResource }
    , m_Size{ bufferResource->size_ }
{
}

BufferBase::BufferBase(BufferBase&& rhs)
    : DeviceChild{ nullptr }
    , m_BufferResource{ nullptr }
    , m_Size{ 0 }
{
    operator=(DRE_MOVE(rhs));
}

BufferBase& BufferBase::operator=(BufferBase&& rhs)
{
    DeviceChild::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_BufferResource);
    DRE_SWAP_MEMBER(m_Size);

    return *this;
}

BufferBase::~BufferBase()
{
    if (m_BufferResource != nullptr)
    {
        m_ParentDevice->GetResourcesController()->FreeBuffer(m_BufferResource);
    }
}

}
