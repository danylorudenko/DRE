#include <engine\data\Material.hpp>

namespace Data
{

Texture2D const& Material::TextureProperty::GetTexture() const
{
    return m_Texture;
}

void Material::TextureProperty::SetTexture(Texture2D&& texture)
{
    m_Texture = DRE_MOVE(texture);
}

void Material::AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture)
{
    m_TextureProperties[slot].SetTexture(DRE_MOVE(texture));
}

Material::Material(char const* name)
    : m_Name{ name }
{

}


}

