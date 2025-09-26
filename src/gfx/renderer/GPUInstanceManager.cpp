#include <gfx\renderer\GPUInstanceManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>

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
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, DRE::U32 globalID, InstanceFlags instanceFlags)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    m_InstanceDataCPU.globalID_instanceFlags = glm::uvec4{ globalID, DRE::U32(instanceFlags), 0, 0 };
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
        m_InstanceDataCPU.globalID_instanceFlags.y |= DRE::U32(flags);
    else
        m_InstanceDataCPU.globalID_instanceFlags.y &= ~DRE::U32(flags);

    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(InstanceFlags flags)
{
    m_InstanceDataCPU.globalID_instanceFlags.y = DRE::U32(flags);
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(MaterialsManager::MaterialGPU const& material)
{
    m_MaterialGPU = material;
    m_InstanceDataCPU.material = reinterpret_cast<S_MATERIAL*>(material.GetAddressGPU());
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}


}