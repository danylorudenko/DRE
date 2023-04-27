#include <engine\data\Geometry.hpp>

namespace Data
{

Geometry::Geometry(std::uint16_t vertexStride, std::uint16_t indexSize)
    : m_VertexStride{ vertexStride }
    , m_IndexSize{ indexSize }
{
}

Geometry::Geometry(Geometry&& rhs)
    : m_VertexStride{ 0 }
    , m_IndexSize{ 0 }
{
    operator=(DRE_MOVE(rhs));
}

Geometry& Geometry::operator=(Geometry&& rhs)
{
    DRE_SWAP_MEMBER(m_VertexStride);
    DRE_SWAP_MEMBER(m_IndexSize);

    DRE_SWAP_MEMBER(m_VertexStorage);
    DRE_SWAP_MEMBER(m_IndexStorage);

    return *this;
}

}

