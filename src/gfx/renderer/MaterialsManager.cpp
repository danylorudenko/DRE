#include <gfx\renderer\MaterialsManager.hpp>

namespace GFX
{

MaterialsManager::MaterialsManager(PersistentStorage* storage)
    : GPUInstanceAllocator<S_MATERIAL, MAX_MATERIALS, 1024>{ storage }
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

void MaterialsManager::MaterialGPU::ScheduleUpdate(glm::ivec4 textureIDs, glm::ivec4 auxTextureIDs, MaterialFlags flags)
{
    m_MaterialDataCPU.texture_common_ids = textureIDs;
    m_MaterialDataCPU.texture_aux_ids = auxTextureIDs;
    m_MaterialDataCPU.flags = glm::ivec4{ static_cast<int>(flags), 0, 0, 0 };
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdateTextures(glm::ivec4 textureIDs)
{
    m_MaterialDataCPU.texture_common_ids = textureIDs;
    Base::Payload::ScheduleUpdate(m_MaterialDataCPU);
}

void MaterialsManager::MaterialGPU::ScheduleUpdateAuxTextures(glm::ivec4 textureIDs)
{
    m_MaterialDataCPU.texture_aux_ids = textureIDs;
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

