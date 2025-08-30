#pragma once

#include <glm\vec4.hpp>

#include <gfx\renderer\MaterialsManager.hpp>

namespace GFX
{

class Material
{
public:
    Material(MaterialsManager::MaterialGPU const& materialGPU);

    void SetCommonTextureIDs(glm::ivec4 const& ids);
    void SetAuxTextureIDs(glm::ivec4 const& ids);
    void SetFlag(MaterialFlags flag, bool enable);
    void SetFlags(MaterialFlags flags);

    inline glm::ivec4 const& GetCommonTextureIDs() const { return m_CommonTextureIDs; }
    inline glm::ivec4 const& GetAuxTextureIDs() const { return m_AuxTextureIDs; }
    inline DRE::U32 GetFlags() const { return m_Flags; }
    inline bool IsFlagEnabled(MaterialFlags flag) const { return (m_Flags & DRE::U32(flag)) != 0; }

    inline MaterialsManager::MaterialGPU& GetMaterialGPU() { return m_MaterialGPU; }

private:
    MaterialsManager::MaterialGPU m_MaterialGPU;
    glm::ivec4 m_CommonTextureIDs;
    glm::ivec4 m_AuxTextureIDs;
    DRE::U32 m_Flags;
};

}

