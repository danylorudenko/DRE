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
    FreeID(static_cast<std::uint16_t>(transform.m_id));
}

std::uint64_t InstanceDataManager::GetBufferAddress() const
{
    return GPUInstanceAllocator::GetBufferAddress();
}

std::uint32_t InstanceDataManager::GetInstanceCount() const
{
    return GPUInstanceAllocator::GetCount();
}

void InstanceDataManager::ScheduleInstanceUpdate(std::uint32_t id, S_INSTANCE const& instanceData)
{
    ScheduleUpdate(id, instanceData);
}

void InstanceDataManager::UpdateGPUInstances(VKW::Context& context)
{
    FlushUpdates(context);
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

InstanceDataManager::InstanceGPU::InstanceGPU(InstanceDataManager* manager, std::uint64_t addressGPU, std::uint32_t id)
    : m_Manager{ manager }
    , m_InstanceDataCPU{}
    , m_AddressGPU{ addressGPU }
    , m_id{ id }
{
}

void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, glm::uvec4 textureIndices, std::uint32_t globalID)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    m_InstanceDataCPU.texture_indicies = textureIndices;
    m_InstanceDataCPU.globalID = glm::uvec4{ globalID, 0, 0, 0 };
    m_Manager->ScheduleInstanceUpdate(m_id, m_InstanceDataCPU);
}


void InstanceDataManager::InstanceGPU::ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform)
{
    m_InstanceDataCPU.world_space = transform;
    m_InstanceDataCPU.inv_world_space = invTransform;
    m_Manager->ScheduleInstanceUpdate(m_id, m_InstanceDataCPU);
}

}