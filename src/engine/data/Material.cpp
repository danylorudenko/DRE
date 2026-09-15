#include <engine\data\Material.hpp>

#include <gfx\renderer\Material.hpp>

namespace Data
{

DRE::String128 const& Material::TextureProperty::GetTextureName() const
{
    return m_GfxTextureName;
}

void Material::TextureProperty::SetGfxTexture(GFX::Texture* texture)
{
    m_GFXTexture = DRE_MOVE(texture);
}

void Material::AssignTextureToSlot(TextureProperty::Slot slot, char const* gfxTextureName, GFX::Texture* gfxTexture)
{
    m_TextureProperties[slot].SetSlotType(slot);
    m_TextureProperties[slot].m_GfxTextureName = gfxTextureName;
    m_TextureProperties[slot].SetGfxTexture(gfxTexture);
}

Material::Material(char const* name)
    : m_Name{ name }
    , m_GFXMaterial{ nullptr }
{
}

void Material::RenderingProperties::SetMaterialType(Material::RenderingProperties::MaterialType type)
{
    m_Type = type;
    if (type == MATERIAL_TYPE_ALPHA_MASKED)
        EnableAlphaMasked(true);
}

void Material::FlushToGfxMaterial(GFX::Material* target)
{
    for (DRE::U32 i = 0; i < TextureProperty::Slot::MAX; i++)
    {
        TextureProperty& property = m_TextureProperties[i];
        if (property.GetSlotType() != TextureProperty::Slot::MAX)
            target->SetTextureNoUpdate(property.GetSlotType(), property.m_GFXTexture);
    }

    target->SetFlagsNoUpdate(m_RenderingProperties.GetMaterialFlags());

    target->FlushDataToMaterialGPU();

    m_GFXMaterial = target;
}

}

