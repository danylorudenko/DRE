#pragma once

#include <cstdint>

#include <gfx\buffer\BufferBase.hpp>

namespace VKW
{
struct BufferResource;
}

namespace GFX
{

class VertexBuffer 
    : public BufferBase
{
public:
    VertexBuffer();
    VertexBuffer(VKW::Device* device, VKW::BufferResource* resource, std::uint32_t vertexCount);

    VertexBuffer(VertexBuffer&& rhs);
    VertexBuffer& operator=(VertexBuffer&& rhs);

    virtual ~VertexBuffer();

private:
    std::uint32_t m_VertexCount;
};

}

