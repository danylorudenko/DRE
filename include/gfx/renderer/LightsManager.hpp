#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

#include <common\lighting\lights.h>

#include <glm\vec3.hpp>

namespace VKW
{
class Context;
class DescriptorManager;
}

namespace GFX
{

class PersistentStorage;

/////////////////////////////
class LightsManager
    : public GPUInstanceAllocator<S_LIGHT, 64, 8>
{
public:
    static constexpr std::uint32_t MAX_LIGHTS = 64;

    using Base = GPUInstanceAllocator<S_LIGHT, 64, 8>;

    class LightGPU : public Base::Payload
    {
        friend class LightsManager;

    public:
        LightGPU(LightsManager* manager, std::uint64_t addressGPU, std::uint16_t id);
        void ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type);
    };

public:
    LightsManager(PersistentStorage* storage);

    LightGPU AllocateLight();
    void FreeLight(LightGPU& light);

    void ScheduleLightUpdate(std::uint16_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type);
    void UpdateGPULights(VKW::Context& context);

    std::uint32_t GetLightsCount() const;

    std::uint64_t GetBufferAddress() const;
};

}

