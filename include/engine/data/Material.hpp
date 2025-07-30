#pragma once

#include <foundation\String\InplaceString.hpp>
#include <foundation\memory\ByteBuffer.hpp>

#include <engine\data\Texture2D.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>

#include <common\instances.h>

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
            //OCCLUSION,
            MAX
        };

        Texture2D const& GetTexture() const;
        void SetTexture(Texture2D&& texture);

    private:
        Texture2D m_Texture;
    };
    //////////////////////////////////////


    //////////////////////////////////////
    // PipelineProperties
    class RenderingProperties
    {
    public:
        enum MaterialType
        {
            MATERIAL_TYPE_OPAQUE,
            MATERIAL_TYPE_WATER,
            MATERIAL_TYPE_MAX
        };

        inline void SetShader(char const* name) { m_Shader = name; }
        inline char const* GetShader() const { return m_Shader.GetData(); }

        inline void SetMaterialType(MaterialType type) { m_Type = type; }
        inline MaterialType GetMaterialType() const { return m_Type; }

        inline void SetInstanceFlags(InstanceFlags flags) { m_InstanceFlags = flags; }
        inline InstanceFlags GetInstanceFlags() const { return m_InstanceFlags; }

        inline void EnableTextureNormals(bool enable) { SetFlag(InstanceFlags::TEXTURE, enable); }
        inline void EnableInvertNormalY(bool enable) { SetFlag(InstanceFlags::TEXTURE_INVERT_Y, enable); }
        inline void EnableTBN(bool enable) { SetFlag(InstanceFlags::TBN, enable); }

        inline bool HasTextureNormals() const { return (m_InstanceFlags & InstanceFlags::TEXTURE) != InstanceFlags{}; }
        inline bool HasInvertNormalY() const { return (m_InstanceFlags & InstanceFlags::TEXTURE_INVERT_Y) != InstanceFlags{}; }
        inline bool HasTBN() const { return (m_InstanceFlags & InstanceFlags::TBN) != InstanceFlags{}; }

    private:
        inline void SetFlag(InstanceFlags flag, bool enable)
        {
            if (enable)
                m_InstanceFlags = InstanceFlags(m_InstanceFlags | flag);
            else
                m_InstanceFlags = InstanceFlags(m_InstanceFlags & ~flag);
        }

        MaterialType m_Type = MATERIAL_TYPE_MAX;
        InstanceFlags m_InstanceFlags = InstanceFlags::TEXTURE;
        DRE::String32 m_Shader;
    };


    Material(char const* name);

    //////////////////////////////////////
    // Material
    void AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture);

    RenderingProperties& GetRenderingProperties() { return m_RenderingProperties; }
    RenderingProperties const& GetRenderingProperties() const { return m_RenderingProperties; }

    inline Texture2D const& GetTexture(TextureProperty::Slot slot) const { return m_TextureProperties[int(slot)].GetTexture(); }

    inline char const* GetName() const { return static_cast<char const*>(m_Name); }

private:
    DRE::String32       m_Name;

    TextureProperty     m_TextureProperties[TextureProperty::Slot::MAX];
    RenderingProperties m_RenderingProperties;
};

}
