#include <gfx\buffer\VertexBuffer.hpp>

#include <foundation\Common.hpp>

namespace GFX
{

VertexBuffer::VertexBuffer()
    : BufferBase{}
    , m_VertexCount{ 0 }
{
}

VertexBuffer::VertexBuffer(VKW::Device* device, VKW::BufferResource* resource, std::uint32_t vertexCount)
    : BufferBase{ device, resource }
    , m_VertexCount{ vertexCount }
{
}

VertexBuffer::VertexBuffer(VertexBuffer&& rhs)
    : BufferBase{}
    , m_VertexCount{ 0 }
{
    operator=(DRE_MOVE(rhs));
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& rhs)
{
    BufferBase::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_VertexCount);

    return *this;
}

VertexBuffer::~VertexBuffer()
{
}

}

