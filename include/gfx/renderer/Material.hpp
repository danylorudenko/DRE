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

    Material(Type type, MaterialsManager::MaterialGPU const& materialGPU);

    virtual ~Material();

    void                    SetTextureNoUpdate(TextureSlot slot, Texture* texture) { m_Textures[slot] = texture; }
    void                    SetFlagsNoUpdate(MaterialFlags flags);

    inline MaterialFlags    GetFlags() const { return MaterialFlags(m_Flags); }
    inline bool             IsFlagEnabled(MaterialFlags flag) const { return (m_Flags & DRE::U32(flag)) != 0; }

    inline Type             GetType() const { return m_MaterialType; }

    inline MaterialsManager::MaterialGPU& GetMaterialGPU() { return m_MaterialGPU; }
    void                    FlushDataToMaterialGPU();

private:
    MaterialsManager::MaterialGPU   m_MaterialGPU;
    Type                            m_MaterialType;

    DRE::U32                        m_Flags; // enum MaterialFlags
    Texture*                        m_Textures[TextureSlot::MAX];
};

}

