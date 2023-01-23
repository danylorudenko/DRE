#include <engine\data\GeometryLibrary.hpp>

namespace Data
{

GeometryLibrary::GeometryLibrary(DRE::DefaultAllocator* allocator)
    : m_Geometries{ allocator }
{
}

void GeometryLibrary::AddGeometry(std::uint32_t id, Geometry&& data)
{
    m_Geometries.Emplace(id, DRE_MOVE(data));
}

}

