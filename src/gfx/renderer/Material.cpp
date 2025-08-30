#include <gfx\renderer\Material.hpp>

namespace GFX
{

Material::Material(MaterialsManager::MaterialGPU const& materialGPU)
    : m_MaterialGPU{ materialGPU }
    , m_CommonTextureIDs{ 0 }
    , m_AuxTextureIDs{ 0 }
    , m_Flags{ 0 }
{
}

void Material::SetCommonTextureIDs(glm::ivec4 const& ids)
{
    m_CommonTextureIDs = ids;
    m_MaterialGPU.ScheduleUpdateTextures(ids);
}

void Material::SetAuxTextureIDs(glm::ivec4 const& ids)
{
    m_AuxTextureIDs = ids;
    m_MaterialGPU.ScheduleUpdateAuxTextures(ids);
}

void Material::SetFlag(MaterialFlags flag, bool enable)
{
    if (enable)
        m_Flags |= static_cast<DRE::U32>(flag);
    else
        m_Flags &= ~static_cast<DRE::U32>(flag);
    m_MaterialGPU.ScheduleUpdate(static_cast<MaterialFlags>(m_Flags));
}

void Material::SetFlags(MaterialFlags flags)
{
    m_Flags = static_cast<DRE::U32>(flags);
    m_MaterialGPU.ScheduleUpdate(flags);
}

} // namespace GFX

