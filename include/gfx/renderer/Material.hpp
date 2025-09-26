#pragma once

#include <glm\vec4.hpp>

#include <gfx\renderer\GPUMaterialsManager.hpp>
#include <gfx\texture\Texture.hpp>
#include <engine\data\Material.hpp>

namespace VKW
{
class Pipeline;
}

namespace GFX
{

class Material
{
public:
    using TextureSlot   = Data::Material::TextureProperty::Slot;
    using Type  = Data::Material::RenderingProperties::MaterialType;

    Material(Type type, MaterialsManager::MaterialGPU const& materialGPU, VKW::Pipeline* pipeline);

    virtual ~Material();

    void        SetTextureNoUpdate(TextureSlot slot, Texture* texture) { m_Textures[slot] = texture; }

    //void        SetDiffuseTextureNoUpdate       (Texture* diffuse)      { m_Diffuse = diffuse; }
    //void        SetNormalTextureNoUpdate        (Texture* normal)       { m_Normal = normal; }
    //void        SetRoughnessTextureNoUpdate     (Texture* roughness)    { m_Roughness = roughness; }
    //void        SetMetalnessTextureNoUpdate     (Texture* metalness)    { m_Metalness = metalness; }
    //void        SetOcclusionTextureNoUpdate     (Texture* occlusion)    { m_Occlusion = occlusion; }
    //void        SetBentNormalTextureNoUpdate    (Texture* bentNormal)   { m_BentNormal = bentNormal; }
    //void        SetAux0TextureNoUpdate          (Texture* aux0)         { m_AuxTexture0 = aux0; }
    //void        SetAux1TextureNoUpdate          (Texture* aux1)         { m_AuxTexture1 = aux1; }
    //
    //Texture*    GetDiffuseTexture   () { return m_Diffuse; }
    //Texture*    GetNormalTexture    () { return m_Normal; }
    //Texture*    GetRoughnessTexture () { return m_Roughness; }
    //Texture*    GetMetalnessTexture () { return m_Metalness; }
    //Texture*    GetOcclusionTexture () { return m_Metalness; }
    //Texture*    GetBentNormalTexture() { return m_Metalness; }
    //Texture*    GetAux0Texture      () { return m_AuxTexture0; }
    //Texture*    GetAux1Texture      () { return m_AuxTexture1; }

    void SetFlagsNoUpdate(MaterialFlags flags);
    inline MaterialFlags GetFlags() const { return MaterialFlags(m_Flags); }
    inline bool IsFlagEnabled(MaterialFlags flag) const { return (m_Flags & DRE::U32(flag)) != 0; }

    inline VKW::Pipeline* GetPipeline() { return m_Pipeline; }
    inline Type GetType() const { return m_MaterialType; }

    inline MaterialsManager::MaterialGPU& GetMaterialGPU() { return m_MaterialGPU; }
    void FlushDataToMaterialGPU();

private:
    MaterialsManager::MaterialGPU   m_MaterialGPU;
    VKW::Pipeline*                  m_Pipeline;
    Type                            m_MaterialType;

    DRE::U32                        m_Flags; // enum MaterialFlags
    Texture*                        m_Textures[TextureSlot::MAX];

    //Texture*            m_Diffuse;
    //Texture*            m_Normal;
    //Texture*            m_Roughness;
    //Texture*            m_Metalness;
    //Texture*            m_Occlusion;
    //Texture*            m_BentNormal;
};

}

