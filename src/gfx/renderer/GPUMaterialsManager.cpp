#include <gfx\renderer\GPUMaterialsManager.hpp>

namespace GFX
{

MaterialsManager::MaterialsManager(PersistentStorage* storage)
    : Base{ storage }
{
}

MaterialsManager::MaterialGPU MaterialsManager::AllocateMaterial()
{
    std::uint16_t const id = AllocateID();
    std::uint64_t addressGPU = GetBufferAddress() + sizeof(S_MATERIAL) * id;
    return MaterialGPU{ this, addressGPU, id };
}

void MaterialsManager::FreeMaterial(MaterialGPU& material)
{
    FreeID(static_cast<std::uint16_t>(material.GetID()));
}

MaterialsManager::MaterialGPU::MaterialGPU(MaterialsManager* manager, DRE::U64 addressGPU, DRE::U32 id)
    : Base::Payload{ manager, addressGPU, static_cast<std::uint16_t>(id) }
    , m_MaterialDataCPU{}
{
}

void MaterialsManager::MaterialGPU::ScheduleUpdate(S_MATERIAL materialData)
{
    m_MaterialDataCPU = materialData;
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdateTextures0(glm::ivec4 commonTextureIDs)
{
    m_MaterialDataCPU.texture_common_ids = commonTextureIDs;
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdateTextures1(glm::ivec4 auxTextureIDs)
{
    m_MaterialDataCPU.texture_aux_ids = auxTextureIDs;
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdate(MaterialFlags flags, bool addFlags)
{
    if (addFlags)
        m_MaterialDataCPU.flags.x |= static_cast<int>(flags);
    else
        m_MaterialDataCPU.flags.x &= ~static_cast<int>(flags);
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdate(MaterialFlags flags)
{
    m_MaterialDataCPU.flags.x = static_cast<int>(flags);
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

} // namespace GFX

