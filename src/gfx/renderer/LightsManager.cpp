#include <gfx\renderer\LightsManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <lights.h>

namespace GFX
{

LightsManager::LightsManager(PersistentStorage* storage)
    : m_PersistentAllocation{ storage->AllocateRegion(MAX_LIGHTS * sizeof(S_LIGHT)) }
    , m_LightsCount{ 0 }
{
}

LightsManager::Light LightsManager::AllocateLight()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();
    ++m_LightsCount;
    return Light{ this, id };
}

void LightsManager::FreeLight(LightsManager::Light& light)
{
    --m_LightsCount;
    m_ElementAllocator.Free(light.m_id);
}

std::uint64_t LightsManager::GetBufferAddress() const
{
    return m_PersistentAllocation.GetGPUAddress();
}

std::uint32_t LightsManager::GetLightsCount() const
{
    return m_LightsCount;
}

void LightsManager::ScheduleLightUpdate(std::uint16_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    S_LIGHT SLight;
    SLight.world_pos = glm::vec4(position, 1.0f);
    SLight.direction_type = glm::vec4(orientation, *reinterpret_cast<float*>(&type));
    SLight.spectrum_flux = glm::vec4(color, flux);

    m_LightUpdateQueue.EmplaceBack(id, SLight);
}

void LightsManager::UpdateGPULights(VKW::Context& context)
{
    std::uint64_t baseAddress = m_PersistentAllocation.GetGPUAddress();

    for (std::uint32_t i = 0, count = m_LightUpdateQueue.Size(); i < count; i++)
    {
        LightUpdateEntry& entry = m_LightUpdateQueue[i];
        m_PersistentAllocation.Update(context, sizeof(S_LIGHT) * entry.id, &entry.payload, sizeof(S_LIGHT));
    }

    m_LightUpdateQueue.Clear();
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

LightsManager::Light::Light(LightsManager* manager, std::uint16_t id)
    : m_LightsManager{ manager }
    , m_id{ id }
{
}

void LightsManager::Light::ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    m_LightsManager->ScheduleLightUpdate(m_id, position, orientation, color, flux, type);
}




}