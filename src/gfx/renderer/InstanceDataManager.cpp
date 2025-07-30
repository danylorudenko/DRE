#include <gfx\renderer\InstanceDataManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

InstanceDataManager::InstanceDataManager(PersistentStorage* storage)
    : GPUInstanceAllocator<S_INSTANCE, 1024 * 32, 1024>{ storage }
{
}

InstanceDataManager::InstanceGPU InstanceDataManager::AllocateTransform()
{
    std::uint16_t const id = AllocateID();

    std::uint64_t addressGPU = GetBufferAddress() + sizeof(S_INSTANCE) * id;
    return InstanceGPU{ this, addressGPU, id };
}

void InstanceDataManager::FreeTransform(InstanceDataManager::InstanceGPU& transform)
{
    FreeID(static_cast<std::uint16_t>(transform.GetID()));
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

InstanceDataManager::InstanceGPU::InstanceGPU(InstanceDataManager* manager, DRE::U64 addressGPU, DRE::U32 id)
    : Base::Payload{ manager, addressGPU, static_cast<std::uint16_t>(id) }
    , m_InstanceDataCPU{}
{
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, glm::uvec4 textureIndices, DRE::U32 globalID, InstanceFlags instanceFlags)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    m_InstanceDataCPU.texture_indicies = textureIndices;
    m_InstanceDataCPU.globalID_instanceFlags = glm::uvec4{ globalID, DRE::U32(instanceFlags), 0, 0 };
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}


void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    Base::Payload::ScheduleUpdate(m_InstanceDataCPU);
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::uvec4 textureIndicies)
{
    m_InstanceDataCPU.texture_indicies = textureIndicies;
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


}