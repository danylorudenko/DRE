#include <gfx\renderer\Material.hpp>
#include <gfx\GraphicsManager.hpp>

namespace GFX
{

Material::Material(Type type, MaterialsManager::MaterialGPU const& materialGPU)
    : m_MaterialGPU{ materialGPU }
    , m_MaterialType{ type }
    , m_Flags{ 0 }
    , m_Textures{} // default-initialize array to zeros
{
}

Material::~Material()
{
    auto* manager = reinterpret_cast<MaterialsManager*>(m_MaterialGPU.GetManager());
    manager->FreeMaterial(m_MaterialGPU);
}

void Material::SetFlagsNoUpdate(MaterialFlags flags)
{
    m_Flags = static_cast<DRE::U32>(flags);
}

void Material::FlushDataToMaterialGPU()
{
    auto TextureID = [this](Data::Material::TextureProperty::Slot slot)
    {
        GFX::Texture* texture = m_Textures[slot];
        if (texture == nullptr)
        {
            texture = GFX::g_GraphicsManager->GetTextureBank().FindTexture(TextureBank::NAME_DEFAULT_BLACK);
        }
        return texture->GetShaderGlobalDescriptor().id_;
    };

    S_MATERIAL data;

    DRE::U32 diffuse    = TextureID(Data::Material::TextureProperty::Slot::DIFFUSE);
    DRE::U32 normal     = TextureID(Data::Material::TextureProperty::Slot::NORMAL);
    DRE::U32 metalness  = TextureID(Data::Material::TextureProperty::Slot::METALNESS);
    DRE::U32 roughness  = TextureID(Data::Material::TextureProperty::Slot::ROUGHNESS);
    DRE::U32 occlusion  = TextureID(Data::Material::TextureProperty::Slot::OCCLUSION);
    DRE::U32 bentNormal = TextureID(Data::Material::TextureProperty::Slot::BENT_NORMAL);

    data.texture_common_ids = uint4{ diffuse, normal, metalness, roughness };
    data.texture_aux_ids = uint4{ occlusion, bentNormal, 0, 0 };
    data.flags = uint4{ m_Flags, 0, 0, 0 };

    m_MaterialGPU.ScheduleUpdate(data);
}

} // namespace GFX

