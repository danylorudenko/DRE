#include <engine\data\MaterialLibrary.hpp>

namespace Data
{

MaterialLibrary::Hash::Hash(std::uint32_t id, const char* sceneName)
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

MaterialLibrary::MaterialLibrary(DRE::DefaultAllocator* allocator)
    : m_MaterialsMap{ allocator }
{}

void MaterialLibrary::InitDefaultMaterials()
{
    //DiffuseWhite,
    //BlinnPhong,
    //CookTorrance,
    //Emissive,

    // soooo, how do I access PipelineFactory
    // maybe some kind of GfxCreationDelegate ?
}

Material* MaterialLibrary::GetMaterial(Hash hash)
{
    return m_MaterialsMap.Find(hash).value;
}

Material* MaterialLibrary::GetMaterial(std::uint32_t id, char const* sceneName)
{
    return GetMaterial(Hash(id, sceneName));
}

Material* MaterialLibrary::CreateMaterial(Hash hash, char const* name)
{
    return &m_MaterialsMap.Emplace(hash, name);
}

Material* MaterialLibrary::CreateMaterial(std::uint32_t id, char const* sceneName, char const* name)
{
    return &m_MaterialsMap.Emplace(Hash(id, sceneName), name);
}

}

