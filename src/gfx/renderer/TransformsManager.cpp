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

TransformsManager::TransformWS TransformsManager::AllocateTransform()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();
    ++m_TransformsCount;
    return TransformWS{ this, id };
}

void TransformsManager::FreeTransform(TransformsManager::TransformWS& transform)
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

void TransformsManager::ScheduleTransformUpdate(std::uint32_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& scale)
{
    S_TRANSFORM STransform;
    //SLight.world_pos = glm::vec4(position, 1.0f);
    //SLight.direction_type = glm::vec4(orientation, *reinterpret_cast<float*>(&type));
    //SLight.spectrum_flux = glm::vec4(color, flux);

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

TransformsManager::TransformWS::TransformWS(TransformsManager* manager, std::uint32_t id)
    : m_TransformsManager{ manager }
    , m_id{ id }
{
}

void TransformsManager::TransformWS::ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    //m_TransformsManager->ScheduleTransformUpdate(m_id, position, orientation, color, flux, type);
}




}