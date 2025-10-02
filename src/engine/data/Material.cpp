#include <engine\data\Material.hpp>

#include <gfx\renderer\Material.hpp>

namespace Data
{

Texture2D const& Material::TextureProperty::GetTexture() const
{
    return m_DataTexture;
}

void Material::TextureProperty::SetDataTexture(Texture2D&& texture)
{
    m_DataTexture = DRE_MOVE(texture);
}

void Material::TextureProperty::SetGfxTexture(GFX::Texture* texture)
{
    m_GFXTexture = DRE_MOVE(texture);
}

void Material::AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture, GFX::Texture* gfxTexture)
{
    m_TextureProperties[slot].SetDataTexture(DRE_MOVE(texture));
    m_TextureProperties[slot].SetGfxTexture(gfxTexture);
}

Material::Material(char const* name)
    : m_Name{ name }
    , m_GFXMaterial{ nullptr }
{
}

void Material::FlushToGfxMaterial(GFX::Material* target)
{
    for (DRE::U32 i = 0; i < TextureProperty::Slot::MAX; i++)
    {
        TextureProperty& property = m_TextureProperties[i];
        if (property.GetSlot() != TextureProperty::Slot::MAX)
            target->SetTextureNoUpdate(property.GetSlot(), property.m_GFXTexture);
    }
    target->SetFlagsNoUpdate(m_RenderingProperties.GetMaterialFlags());

    target->FlushDataToMaterialGPU();

    m_GFXMaterial = target;
}

}

