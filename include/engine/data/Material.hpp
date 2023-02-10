#pragma once

#include <foundation\String\InplaceString.hpp>
#include <foundation\memory\ByteBuffer.hpp>

#include <engine\data\Texture2D.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>

namespace Data
{

class Material
{
public:
    //////////////////////////////////////
    // TextureProperty
    class TextureProperty
    {
    public:
        friend class Material;

        enum Slot
        {
            DIFFUSE,
            NORMAL,
            METALNESS,
            ROUGHNESS,
            OCCLUSION,
            MAX
        };

        Texture2D const& GetTexture() const;
        void SetTexture(Texture2D&& texture);

    private:
        Texture2D m_Texture;
    };
    //////////////////////////////////////


    //////////////////////////////////////
    // DataProperty
    class DataProperty
    {
    public:
        friend class Material;

        void AssignData(void const* data, DRE::SizeT size);
        template<typename T>
        void AssignData(T const& data) { AssignData(&data, sizeof(data)); }

        void* GetData() const;

    private:
        DRE::ByteBuffer m_DataBuffer;
    };
    //////////////////////////////////////


    //////////////////////////////////////
    // PipelineProperties
    class RenderingProperties
    {
    public:
        enum MaterialType
        {
            MATERIAL_TYPE_DEFAULT_LIT,
            MATERIAL_TYPE_COOK_TORRANCE,
            MATERIAL_TYPE_MAX
        };

        inline void SetMaterialType(MaterialType type) { m_Type = type; }
        inline MaterialType GetMaterialType() const { return m_Type; }

    private:
        MaterialType m_Type = MATERIAL_TYPE_MAX;
    };


    Material(char const* name);

    //////////////////////////////////////
    // Material
    void AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture);
    void AssignData(void const* data, DRE::SizeT size);
    template<typename T>
    void AssignData(T const& data) { m_DataProperty.AssignData(data); }

    RenderingProperties& GetRenderingProperties() { return m_RenderingProperties; }
    RenderingProperties const& GetRenderingProperties() const { return m_RenderingProperties; }

    inline Texture2D const& GetTexture(TextureProperty::Slot slot) const { return m_TextureProperties[int(slot)].GetTexture(); }

    inline char const* GetName() const { return m_Name; }

private:
    DRE::String32       m_Name;

    TextureProperty     m_TextureProperties[TextureProperty::Slot::MAX];
    DataProperty        m_DataProperty;
    RenderingProperties m_RenderingProperties;
};

}
