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

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

LightsManager::LightGPU::LightGPU(LightsManager* manager, std::uint64_t addressGPU, std::uint16_t id)
    : Base::Payload{ manager, addressGPU, id }
{
}

void LightsManager::LightGPU::ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type)
{
    S_LIGHT SLight;
    SLight.world_pos = glm::vec4(position, 1.0f);
    SLight.direction_type = glm::vec4(orientation, *reinterpret_cast<float*>(&type));
    SLight.spectrum_flux = glm::vec4(color, flux);

    Base::Payload::ScheduleUpdate(SLight);
}




}