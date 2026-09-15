#include <engine\data\MaterialLibrary.hpp>
#include <gfx\GraphicsManager.hpp>

namespace Data
{

MaterialLibrary::Hash::Hash(DRE::U32 id, const char* sceneName)
{
    DRE::U32 buffer[32];
    DRE::MemZero(buffer, sizeof(buffer));

    void* ptr = buffer;
    DRE::WriteMemorySequence(ptr, id);

    DRE::U32 const nameSize = std::strlen(sceneName);
    DRE_ASSERT(nameSize < sizeof(buffer) - 5, "GeometryLibrary doesn't support big file names.");
    DRE::WriteMemorySequence(ptr, sceneName, nameSize);

    m_Hash = fasthash32(buffer, sizeof(buffer), DRE::U32(0xE527A10B));
}

MaterialLibrary::Hash::Hash(const char* name)
{
    m_Hash = fasthash32(name, std::strlen(name), DRE::U32(0xE527A10B));
}

MaterialLibrary::MaterialLibrary(DRE::DefaultAllocator* allocator)
    : m_MaterialsMap{ allocator }
    , m_MaterialIDCounter{ 0 }
{}

void MaterialLibrary::InitDefaultMaterials()
{
    Material* defaultDiffuseMaterial = CreateMaterial(NAME_DEFAULT_DIFFUSE_WHITE);
    defaultDiffuseMaterial->GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_OPAQUE);
    defaultDiffuseMaterial->GetRenderingProperties().EnableNormalTBN(true);

    GFX::Texture* whiteTexture = GFX::g_GraphicsManager->GetTextureBank().FindTexture(GFX::TextureBank::NAME_DEFAULT_WHITE);
    defaultDiffuseMaterial->AssignTextureToSlot(Material::TextureProperty::DIFFUSE, GFX::TextureBank::NAME_DEFAULT_WHITE, whiteTexture);

    GFX::Material* gfxMaterial = GFX::g_GraphicsManager->CreateMaterial(defaultDiffuseMaterial->GetName(), GFX::Material::Type::MATERIAL_TYPE_OPAQUE);
    defaultDiffuseMaterial->FlushToGfxMaterial(gfxMaterial);
}

Material* MaterialLibrary::GetMaterial(Hash hash)
{
    return m_MaterialsMap.Find(hash).value;
}

Material* MaterialLibrary::GetMaterial(DRE::U32 id, char const* sceneName)
{
    return GetMaterial(Hash(id, sceneName));
}

Material* MaterialLibrary::GetMaterial(char const* name)
{
    return GetMaterial(Hash(name));
}

Material* MaterialLibrary::CreateMaterial(char const* name)
{
    return CreateMaterial(Hash(name), name);
}

Material* MaterialLibrary::CreateMaterial(Hash hash, char const* name)
{
    DRE_ASSERT(m_MaterialsMap.Find(hash).value == nullptr, "DATA::Material already exists.");
    return &m_MaterialsMap.Emplace(hash, name);
}

Material* MaterialLibrary::CreateMaterial(DRE::U32 id, char const* sceneName, char const* name)
{
    DRE::String64 materialName;
    materialName.Append(sceneName);
    materialName.Append("_");
    materialName.Append(name);
    DRE::U32 uniqueNameID = m_MaterialIDCounter++;
    materialName.AppendFormat("%u", uniqueNameID);

    return CreateMaterial(Hash(id, sceneName), materialName.GetData());
}

}

