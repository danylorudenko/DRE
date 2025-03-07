#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>

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
    : public NonMovable
    , public NonCopyable
{
public:
    static constexpr std::uint32_t MAX_INSTANCES = 1024 * 32;

    class InstanceGPU
    {
        friend class InstanceDataManager;

    public:
        InstanceGPU(InstanceDataManager* manager, std::uint64_t addressGPU, std::uint32_t id);
        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, glm::uvec4 textureIndices, std::uint32_t globalID);
        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform);

        std::uint32_t GetID() const { return m_id; }
        std::uint64_t GetAddressGPU() const { return m_AddressGPU; }
        glm::mat4 const& GetTransform() const { return m_InstanceDataCPU.world_space; }


    private:
        InstanceDataManager*    m_Manager;

        S_INSTANCE              m_InstanceDataCPU;

        std::uint64_t           m_AddressGPU;
        std::uint32_t           m_id;
    };

public:
    InstanceDataManager(PersistentStorage* storage);

    InstanceGPU AllocateTransform();
    void FreeTransform(InstanceGPU& transform);

    void UpdateGPUInstances(VKW::Context& context);

    std::uint32_t GetInstanceCount() const;
    std::uint64_t GetBufferAddress() const;

private:
    friend class InstanceGPU;
    void ScheduleInstanceUpdate(std::uint32_t id, S_INSTANCE const& transform);

private:
    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListOffsetAllocator<MAX_INSTANCES> m_ElementAllocator;
    std::uint32_t m_InstancesCount;

    struct InstanceUpdateEntry
    {
        std::uint32_t   id;
        S_INSTANCE      payload;
    };
    DRE::InplaceVector<InstanceUpdateEntry, 1024> m_UpdateQueue;
};

}

