#include <data\MaterialLibrary.hpp>

namespace Data
{

MaterialLibrary::MaterialLibrary()
    : m_MaterialsMap{ &DRE::g_MainAllocator }
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

Material* MaterialLibrary::GetMaterial(char const* name)
{
    return m_MaterialsMap.Find(name).value;
}

Material* MaterialLibrary::CreateMaterial(char const* name)
{
    return &m_MaterialsMap.Emplace(name, name);
}

}

