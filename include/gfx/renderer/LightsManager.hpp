#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\ElementAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

#include <lights.h>

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
    : public NonMovable
    , public NonCopyable
{
public:
    static constexpr std::uint32_t MAX_LIGHTS = 64;

    class Light
    {
        friend class LightsManager;

    public:
        void ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type);

    private:
        Light(LightsManager* manager, std::uint16_t id);

        LightsManager*  m_LightsManager;
        std::uint16_t   m_id;
    };

public:
    LightsManager(PersistentStorage* storage);

    Light AllocateLight();
    void FreeLight(Light& light);

    void ScheduleLightUpdate(std::uint16_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type);
    void UpdateGPULights(VKW::Context& context);

    std::uint32_t GetLightsCount() const;

    std::uint64_t GetBufferAddress() const;


private:
    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListElementAllocator<MAX_LIGHTS> m_ElementAllocator;
    std::uint32_t m_LightsCount;

    struct LightUpdateEntry
    {
        std::uint16_t id;
        S_LIGHT payload;
    };
    DRE::InplaceVector<LightUpdateEntry, 8> m_LightUpdateQueue;
};

}

