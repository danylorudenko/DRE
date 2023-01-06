#pragma once

#include <foundation\class_features\NonCopyable.hpp>

namespace VKW
{
class Device;
}

namespace GFX
{

//////////////////////////////////////
// class DeviceChild
class DeviceChild : public NonCopyable
{
public:
    DeviceChild(VKW::Device* device);
    DeviceChild(DeviceChild&& rhs);
    DeviceChild& operator=(DeviceChild&& rhs);

    virtual ~DeviceChild() {}

protected:
    VKW::Device* m_ParentDevice;
};

}
