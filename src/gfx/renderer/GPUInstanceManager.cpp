#include <gfx\renderer\GPUInstanceManager.hpp>

namespace GFX
{

InstanceDataManager::InstanceDataManager(PersistentStorage* storage)
    : Base{ storage }
{
}

InstanceDataManager::InstanceGPU InstanceDataManager::AllocateInstance(MaterialsManager::MaterialGPU const& material)
{
    std::uint16_t const id = AllocateID();

    std::uint64_t addressGPU = GetBufferAddress() + sizeof(S_INSTANCE) * id;
    return InstanceGPU{ this, addressGPU, id, InstanceFlags{ 0 }, material };
}

void InstanceDataManager::FreeInstance(InstanceDataManager::InstanceGPU& instance)
{
    FreeID(instance.GetID());
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

InstanceDataManager::InstanceGPU::InstanceGPU(InstanceDataManager* manager, DRE::U64 addressGPU, DRE::U32 id, InstanceFlags flags, MaterialsManager::MaterialGPU const& material)
    : Base::Payload{ manager, addressGPU, static_cast<std::uint16_t>(id) }
    , m_InstanceDataCPU{}
    , m_MaterialGPU{ material }
{
    m_InstanceDataCPU.material = reinterpret_cast<S_MATERIAL*>(m_MaterialGPU.GetAddressGPU());
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, DRE::U32 sceneNodeID, InstanceFlags instanceFlags, DRE::U32 indexOffset, DRE::U32 vertexOffset)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    m_InstanceDataCPU.sceneNodeID = sceneNodeID;
    m_InstanceDataCPU.instanceFlags = DRE::U32(instanceFlags);
    m_InstanceDataCPU.indexOffset = indexOffset;
    m_InstanceDataCPU.vertexOffset = vertexOffset;

    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}


void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(InstanceFlags flags, bool addFlags)
{
    if (addFlags)
        m_InstanceDataCPU.instanceFlags |= DRE::U32(flags);
    else
        m_InstanceDataCPU.instanceFlags &= ~DRE::U32(flags);

    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(InstanceFlags flags)
{
    m_InstanceDataCPU.instanceFlags = DRE::U32(flags);
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(MaterialsManager::MaterialGPU const& material)
{
    m_MaterialGPU = material;
    m_InstanceDataCPU.material = reinterpret_cast<S_MATERIAL*>(material.GetAddressGPU());
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}


}