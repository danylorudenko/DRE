#include <gfx\buffer\StorageBuffer.hpp>

#include <foundation\Common.hpp>

namespace GFX
{

StorageBuffer::StorageBuffer()
    : BufferBase{}
{
}

StorageBuffer::StorageBuffer(VKW::Device* device, VKW::BufferResource* buffer)
    : BufferBase{ device, buffer }
{
}

StorageBuffer::StorageBuffer(StorageBuffer&& rhs)
    : BufferBase{}
{
    operator=(DRE_MOVE(rhs));
}

StorageBuffer& StorageBuffer::operator=(StorageBuffer&& rhs)
{
    BufferBase::operator=(DRE_MOVE(rhs));

    return *this;
}


StorageBuffer::~StorageBuffer()
{
}

}

