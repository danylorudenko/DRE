#include <engine\data\GeometryLibrary.hpp>

#include <foundation\util\Hash.hpp>

namespace Data
{

GeometryLibrary::Hash::Hash(std::uint32_t id, const char* sceneName)
{
    std::uint32_t buffer[32];
    DRE::MemZero(buffer, sizeof(buffer));

    void* ptr = buffer;
    DRE::WriteMemorySequence(ptr, id);

    std::uint32_t const nameSize = std::strlen(sceneName);
    DRE_ASSERT(nameSize < sizeof(buffer) - 5, "GeometryLibrary doesn't support big file names.");
    DRE::WriteMemorySequence(ptr, sceneName, nameSize);

    m_Hash = fasthash32(buffer, sizeof(buffer), uint32_t(0xE527A10B));
}

GeometryLibrary::GeometryLibrary(DRE::DefaultAllocator* allocator)
    : m_Geometries{ allocator }
{
}

void GeometryLibrary::AddGeometry(std::uint32_t id, char const* sceneName, Geometry&& data)
{
    AddGeometry(Hash(id, sceneName), DRE_MOVE(data));
}

void GeometryLibrary::AddGeometry(Hash hash, Geometry&& data)
{
    m_Geometries.Emplace(hash, DRE_MOVE(data));
}


}

