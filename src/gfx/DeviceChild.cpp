#include <gfx\DeviceChild.hpp>

#include <foundation\Common.hpp>

namespace GFX
{

DeviceChild::DeviceChild(VKW::Device* device)
    : m_ParentDevice(device)
{}

DeviceChild::DeviceChild(DeviceChild&& rhs)
    : m_ParentDevice{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

DeviceChild& DeviceChild::operator=(DeviceChild&& rhs)
{
    DRE_SWAP_MEMBER(m_ParentDevice);
    return *this;
}

}
