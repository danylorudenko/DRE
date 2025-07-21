#include <gfx\renderer\LightsManager.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <gfx\GraphicsManager.hpp>
#include <common\lighting\lights.h>

namespace GFX
{

LightsManager::LightsManager(PersistentStorage* storage)
    : GPUInstanceAllocator<S_LIGHT, 64, 8>{ storage }
{
}

LightsManager::LightGPU LightsManager::AllocateLight()
{
    std::uint16_t const id = AllocateID();
    std::uint64_t address = GetBufferAddress() + sizeof(S_LIGHT) * id;
    return LightGPU{ this, address, id };
}

void LightsManager::FreeLight(LightsManager::LightGPU& light)
{
    FreeID(static_cast<std::uint16_t>(light.GetID()));
}

std::uint64_t LightsManager::GetBufferAddress() const
{
    return GPUInstanceAllocator::GetBufferAddress();
}

std::uint32_t LightsManager::GetLightsCount() const
{
    return GPUInstanceAllocator::GetCount();
}

void LightsManager::ScheduleLightUpdate(std::uint16_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    S_LIGHT SLight;
    SLight.world_pos = glm::vec4(position, 1.0f);
    SLight.direction_type = glm::vec4(orientation, *reinterpret_cast<float*>(&type));
    SLight.spectrum_flux = glm::vec4(color, flux);

    ScheduleUpdate(id, SLight);
}

void LightsManager::UpdateGPULights(VKW::Context& context)
{
    FlushUpdates(context);
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

LightsManager::LightGPU::LightGPU(LightsManager* manager, std::uint64_t addressGPU, std::uint16_t id)
    : Base::Payload{ manager, addressGPU, id }
{
}

void LightsManager::LightGPU::ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    m_Manager->ScheduleLightUpdate(static_cast<std::uint16_t>(m_id), position, orientation, color, flux, type);
}




}