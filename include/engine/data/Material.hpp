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
            MATERIAL_TYPE_WATER,
            MATERIAL_TYPE_MAX
        };

        inline void SetShader(char const* name) { m_Shader = name; }
        inline char const* GetShader() const { return m_Shader.GetData(); }

        inline void SetMaterialType(MaterialType type) { m_Type = type; }
        inline MaterialType GetMaterialType() const { return m_Type; }

        inline void SetMaterialFlags(MaterialFlags flags) { m_MaterialFlags = flags; }
        inline MaterialFlags GetMaterialFlags() const { return m_MaterialFlags; }

        inline void EnableNormalTexture(bool enable)                { SetFlag(MATERIAL_FLAG_NORMAL_TEXTURE, enable); }
        inline void EnableNormalInvertY(bool enable)                { SetFlag(MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y, enable); }
        inline void EnableNormalTBN(bool enable)                    { SetFlag(MATERIAL_FLAG_NORMAL_TBN, enable); }
        inline void EnableMaterialTexturesDefault(bool enable)      { SetFlag(MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT, enable); }
        inline void EnableMaterialTexturesGLTFSpheres(bool enable)  { SetFlag(MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES, enable); }

        inline bool HasNormalTexture() const                { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TEXTURE) != 0; }
        inline bool HasNormalTextureInvertY() const         { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0; }
        inline bool HasNormalTBN() const                    { return (m_MaterialFlags & MATERIAL_FLAG_NORMAL_TBN) != 0; }
        inline bool HasMaterialTexturesDefault() const      { return (m_MaterialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT) != 0; }
        inline bool HasMaterialTexturesGLTFSpheres() const  { return (m_MaterialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES) != 0; }

    private:
        inline void SetFlag(MaterialFlags flag, bool enable)
        {
            if (enable)
                m_MaterialFlags = MaterialFlags(m_MaterialFlags | flag);
            else
                m_MaterialFlags = MaterialFlags(m_MaterialFlags & ~flag);
        }

        MaterialType m_Type = MATERIAL_TYPE_MAX;
        MaterialFlags m_MaterialFlags = MATERIAL_FLAG_NORMAL_TEXTURE;
        DRE::String32 m_Shader;
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

    DRE::String32                       m_Name;
    GFX::Material*                      m_GFXMaterial;

    TextureProperty     m_TextureProperties[TextureProperty::Slot::MAX];
    RenderingProperties m_RenderingProperties;
};

}
