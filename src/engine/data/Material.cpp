#include <data\Material.hpp>

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

void Material::DataProperty::AssignData(void const* data, DRE::SizeT size)
{
    m_DataBuffer.Resize(size);
    std::memcpy(m_DataBuffer.Data(), data, size);
}

void* Material::DataProperty::GetData() const
{
    return m_DataBuffer.Data();
}

void Material::AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture)
{
    m_TextureProperties[slot].SetTexture(DRE_MOVE(texture));
}

void Material::AssignData(void const* data, DRE::SizeT size)
{
    m_DataProperty.AssignData(data, size);
}

Material::Material(char const* name)
    : m_Name{ name }
{

}


}

