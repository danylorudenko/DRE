#include <engine\data\Geometry.hpp>

namespace Data
{

Geometry::Geometry(DRE::DefaultAllocator* allocator)
    : m_VertexStorage{ allocator }
    , m_IndexStorage{ allocator }
{
}

Geometry::Geometry(Geometry&& rhs)
    : m_VertexStorage{ nullptr }
    , m_IndexStorage{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

Geometry& Geometry::operator=(Geometry&& rhs)
{
    DRE_SWAP_MEMBER(m_VertexStorage);
    DRE_SWAP_MEMBER(m_IndexStorage);

    return *this;
}

}

