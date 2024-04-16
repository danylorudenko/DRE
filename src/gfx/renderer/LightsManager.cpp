#include <gfx\renderer\LightsManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
//#include <gfx\buffer\PersistentStorage.hpp>
#include <lights.h>

namespace GFX
{

LightsManager::LightsManager(PersistentStorage* storage)
    : m_PersistentAllocation{ storage->AllocateRegion(MAX_LIGHTS * sizeof(S_LIGHT)) }
{
}

LightsManager::Light LightsManager::AllocateLight()
{
    std::uint16_t const id = m_ElementAllocator.Allocate();

    return Light{ m_PersistentAllocation, id };
}

void LightsManager::FreeLight(LightsManager::Light& light)
{
    m_ElementAllocator.Free(light.m_id);
}

std::uint64_t LightsManager::GetBufferAddress() const
{
    return m_PersistentAllocation.GetGPUAddress();
}

LightsManager::Light::Light(PersistentStorage::Allocation allocation, std::uint16_t id)
    : m_Allocation{ allocation }
    , m_id{ id }
{

}

void LightsManager::Light::Update(VKW::Context& context, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    S_LIGHT SLight;
    SLight.world_pos = glm::vec4(position, 1.0f);
    SLight.direction = glm::vec4(orientation, *reinterpret_cast<float*>(&type));
    SLight.radiant_flux_spectrum = glm::vec4(color, 0.0f) * flux;

    m_Allocation.Update(context, SLight);
}


}