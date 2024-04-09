#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\ElementAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

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
        void Update(VKW::Context& context, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux);

    private:
        Light(PersistentStorage::Allocation allocation, std::uint16_t id);

        PersistentStorage::Allocation m_Allocation;
        std::uint16_t m_id;
    };

public:
    LightsManager(PersistentStorage* storage);

    Light AllocateLight();
    void FreeLight(Light& light);

    std::uint64_t GetBufferAddress() const;


private:
    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListElementAllocator<MAX_LIGHTS> m_ElementAllocator;
};

}

