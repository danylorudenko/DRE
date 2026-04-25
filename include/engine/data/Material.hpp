#pragma once

#include <foundation\String\InplaceString.hpp>
#include <foundation\memory\ByteBuffer.hpp>

#include <engine\data\Texture2D.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>

#include <gfx\texture\Texture.hpp>

#include <common\materials.h>

namespace GFX
{
class Material;
class Texture;
}

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
            BENT_NORMAL,
            OPACITY,
            GLOSSINESS,
            SPECULAR,
            BUMP,
            MAX
        };

        Texture2D const& GetTexture() const;
        void SetDataTexture(Texture2D&& texture);
        void SetGfxTexture(GFX::Texture* texture);

        inline Slot GetSlotType() const { return m_Slot; }
        inline void SetSlotType(Slot slot) { m_Slot = slot; }

    private:
        Slot            m_Slot = Slot::MAX;
        Texture2D       m_DataTexture;
        GFX::Texture*   m_GFXTexture = nullptr; // TODO: should receive dummy black texture if slot is empty

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
            MATERIAL_TYPE_ALPHA_MASKED,
            MATERIAL_TYPE_WATER,
            MATERIAL_TYPE_MAX
        };

        void SetMaterialType(MaterialType type);
        inline MaterialType GetMaterialType() const { return m_Type; }

        inline void SetMaterialFlags(MaterialFlags flags) { m_MaterialFlags = flags; }
        inline MaterialFlags GetMaterialFlags() const { return m_MaterialFlags; }

        inline void EnableUsePBRTextures(bool enable)                               { SetFlag(MATERIAL_FLAG_USE_PBR, enable); }
        inline void EnableUseSpecGloss(bool enable)                                 { SetFlag(MATERIAL_FLAG_USE_SPEC_GLOSS, enable); }
        inline void EnableNormalTexture(bool enable)                                { SetFlag(MATERIAL_FLAG_NORMAL_TEXTURE, enable); }
        inline void EnableNormalInvertY(bool enable)                                { SetFlag(MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y, enable); }
        inline void EnableNormalTBN(bool enable)                                    { SetFlag(MATERIAL_FLAG_NORMAL_TBN, enable); }
        inline void EnableMaterialTexturesMetallicRoughnessCombined(bool enable)    { SetFlag(MATERIAL_FLAG_METALLIC_ROUGNESS_COMBINED, enable); }
        inline void EnableAlphaMasked(bool enable)                                  { SetFlag(MATERIAL_FLAG_ALPHA_MASKED, enable); }
        inline void EnableBumpTexture(bool enable)                                  { SetFlag(MATERIAL_FLAG_BUMP_TEXTURE, enable); }

        inline bool HasUsePBRTextures() const                                       { return (m_MaterialFlags & MATERIAL_FLAG_USE_PBR) != 0; }
        inline bool HasUseSpecGloss() const                                         { return (m_MaterialFlags & MATERIAL_FLAG_USE_SPEC_GLOSS) != 0; }
        inline bool HasNormalTexture() const                                        { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TEXTURE) != 0; }
        inline bool HasNormalTextureInvertY() const                                 { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0; }
        inline bool HasNormalTBN() const                                            { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TBN) != 0; }
        inline bool HasMaterialTexturesMetallicRoughnessCombined() const            { return (m_MaterialFlags & MATERIAL_FLAG_METALLIC_ROUGNESS_COMBINED) != 0; }
        inline bool HasAlphaMasked() const                                          { return (m_MaterialFlags & MATERIAL_FLAG_ALPHA_MASKED) != 0; }
        inline bool HasBumpTexture() const                                          { return (m_MaterialFlags & MATERIAL_FLAG_BUMP_TEXTURE) != 0; }

    private:
        inline void SetFlag(MaterialFlags flag, bool enable)
        {
            if (enable)
                m_MaterialFlags = MaterialFlags(m_MaterialFlags | flag);
            else
                m_MaterialFlags = MaterialFlags(m_MaterialFlags & ~flag);
        }

        MaterialType m_Type = MATERIAL_TYPE_MAX;
        MaterialFlags m_MaterialFlags = 0;
    };


    Material(char const* name);

    //////////////////////////////////////
    // Material
    void AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture, GFX::Texture* gfxTexture);

    RenderingProperties& GetRenderingProperties() { return m_RenderingProperties; }
    RenderingProperties const& GetRenderingProperties() const { return m_RenderingProperties; }

    inline Texture2D const& GetTexture(TextureProperty::Slot slot) const { return m_TextureProperties[int(slot)].GetTexture(); }
    inline GFX::Material* GetGfxMaterial() { return m_GFXMaterial; }

    inline char const* GetName() const { return static_cast<char const*>(m_Name); }

    void FlushToGfxMaterial(GFX::Material* target);

private:

    DRE::String64                       m_Name;
    GFX::Material*                      m_GFXMaterial;

    TextureProperty     m_TextureProperties[TextureProperty::Slot::MAX];
    RenderingProperties m_RenderingProperties;
};

}
