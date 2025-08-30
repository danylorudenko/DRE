#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>
#include <gfx\renderer\GPUInstanceAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

#include <common\instances.h>

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
class InstanceDataManager
    : public GPUInstanceAllocator<S_INSTANCE, 1024 * 32, 1024>
{
public:
    static constexpr std::uint32_t MAX_INSTANCES = 1024 * 32;

    using Base = GPUInstanceAllocator<S_INSTANCE, MAX_INSTANCES, 1024>;

    class InstanceGPU : public Base::Payload
    {
        friend class InstanceDataManager;

    public:
        InstanceGPU(InstanceDataManager* manager, DRE::U64 addressGPU, DRE::U32 id);

        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, glm::uvec4 textureIndices, DRE::U32 globalID, InstanceFlags instanceFlags);
        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform);
        void ScheduleUpdate(glm::uvec4 textureIndicies);
        void ScheduleUpdate(InstanceFlags flags, bool addFlags);
        void ScheduleUpdate(InstanceFlags flags);
        void ScheduleUpdate(S_MATERIAL* material);

        glm::mat4 const& GetTransform() const { return m_InstanceDataCPU.world_space; }

    private:
        S_INSTANCE m_InstanceDataCPU;
    };

public:
    InstanceDataManager(PersistentStorage* storage);

    InstanceGPU AllocateTransform();
    void FreeTransform(InstanceGPU& transform);
private:
    friend class InstanceGPU;
};

}

