#include <gfx\renderer\TransformsManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>

namespace GFX
{

TransformsManager::TransformsManager(PersistentStorage* storage)
    : m_PersistentAllocation{ storage->AllocateRegion(MAX_TRANSFORMS * sizeof(S_TRANSFORM)) }
    , m_TransformsCount{ 0 }
{
}

TransformsManager::TransformGPU TransformsManager::AllocateTransform()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();
    ++m_TransformsCount;

    std::uint64_t addressGPU = m_PersistentAllocation.GetGPUAddress() + sizeof(S_TRANSFORM) * id;
    return TransformGPU{ this, addressGPU, id };
}

void TransformsManager::FreeTransform(TransformsManager::TransformGPU& transform)
{
    --m_TransformsCount;
    m_ElementAllocator.Free(transform.m_id);
}

std::uint64_t TransformsManager::GetBufferAddress() const
{
    return m_PersistentAllocation.GetGPUAddress();
}

std::uint32_t TransformsManager::GetTransformsCount() const
{
    return m_TransformsCount;
}

void TransformsManager::ScheduleTransformUpdate(std::uint32_t id, glm::mat4 const& worldSpace)
{
    S_TRANSFORM STransform;
    STransform.world_space = worldSpace;

    m_TransformUpdateQueue.EmplaceBack(id, STransform);
}

void TransformsManager::UpdateGPUTransforms(VKW::Context& context)
{
    std::uint64_t baseAddress = m_PersistentAllocation.GetGPUAddress();

    for (std::uint32_t i = 0, count = m_TransformUpdateQueue.Size(); i < count; i++)
    {
        TransformUpdateEntry& entry = m_TransformUpdateQueue[i];
        m_PersistentAllocation.Update(context, sizeof(S_TRANSFORM) * entry.id, &entry.payload, sizeof(S_TRANSFORM));
    }

    m_TransformUpdateQueue.Clear();
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

TransformsManager::TransformGPU::TransformGPU(TransformsManager* manager, std::uint64_t addressGPU, std::uint32_t id)
    : m_TransformsManager{ manager }
    , m_AddressGPU{ addressGPU }
    , m_id{ id }
{
}

void TransformsManager::TransformGPU::ScheduleUpdate(glm::mat4 worldSpace)
{
    m_TransformsManager->ScheduleTransformUpdate(m_id, worldSpace);
}

std::uint64_t TransformsManager::TransformGPU::GetAddressGPU() const
{
    return m_AddressGPU;
}


}