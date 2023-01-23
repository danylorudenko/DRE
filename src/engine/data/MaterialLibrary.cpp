#include <engine\data\MaterialLibrary.hpp>

namespace Data
{

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

Material* MaterialLibrary::GetMaterial(std::uint32_t id)
{
    return m_MaterialsMap.Find(id).value;
}

Material* MaterialLibrary::CreateMaterial(std::uint32_t id, char const* name)
{
    return &m_MaterialsMap.Emplace(id, name);
}

}

