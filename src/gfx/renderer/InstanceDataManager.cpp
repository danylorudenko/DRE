#include <gfx\renderer\InstanceDataManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

InstanceDataManager::InstanceDataManager(PersistentStorage* storage)
    : m_PersistentAllocation{ storage->AllocateRegion(MAX_INSTANCES * sizeof(S_INSTANCE)) }
    , m_InstancesCount{ 0 }
{
}

InstanceDataManager::InstanceGPU InstanceDataManager::AllocateTransform()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();
    ++m_InstancesCount;

    std::uint64_t addressGPU = m_PersistentAllocation.GetGPUAddress() + sizeof(S_INSTANCE) * id;
    return InstanceGPU{ this, addressGPU, id };
}

void InstanceDataManager::FreeTransform(InstanceDataManager::InstanceGPU& transform)
{
    --m_InstancesCount;
    m_ElementAllocator.Free(transform.m_id);
}

std::uint64_t InstanceDataManager::GetBufferAddress() const
{
    return m_PersistentAllocation.GetGPUAddress();
}

std::uint32_t InstanceDataManager::GetInstanceCount() const
{
    return m_InstancesCount;
}

void InstanceDataManager::ScheduleInstanceUpdate(std::uint32_t id, S_INSTANCE const& instanceData)
{
    m_UpdateQueue.EmplaceBack(id, instanceData);
}

void InstanceDataManager::UpdateGPUInstances(VKW::Context& context)
{
    std::uint64_t baseAddress = m_PersistentAllocation.GetGPUAddress();

    for (std::uint32_t i = 0, count = m_UpdateQueue.Size(); i < count; i++)
    {
        InstanceUpdateEntry& entry = m_UpdateQueue[i];
        m_PersistentAllocation.Update(context, entry.id * sizeof(S_INSTANCE), &entry.payload, sizeof(S_INSTANCE));
    }

    m_UpdateQueue.Clear();
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


}