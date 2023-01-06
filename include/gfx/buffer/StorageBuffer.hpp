#pragma once

#include <gfx\buffer\BufferBase.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>

namespace GFX
{

class StorageBuffer
    : public BufferBase
{
public:
    StorageBuffer();
    StorageBuffer(VKW::Device* device, VKW::BufferResource* buffer);

    StorageBuffer(StorageBuffer&& rhs);
    StorageBuffer& operator=(StorageBuffer&& rhs);

    virtual ~StorageBuffer();
};

}

