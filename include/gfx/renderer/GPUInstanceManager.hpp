#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>
#include <gfx\renderer\GPUInstanceAllocator.hpp>
#include <gfx\renderer\GPUMaterialsManager.hpp>

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


constexpr DRE::U32 MAX_GPU_INSTANCES            = 1024 * 32;
constexpr DRE::U32 MAX_GPU_INSTANCES_QUEUE      = 4096;

using InstanceDataManagerBase = GPUInstanceAllocator<S_INSTANCE, MAX_GPU_INSTANCES, MAX_GPU_INSTANCES_QUEUE>;


/////////////////////////////
class InstanceDataManager
    : public InstanceDataManagerBase
{
public:
    using Base = InstanceDataManagerBase;

    class InstanceGPU : public Base::Payload
    {
        friend class InstanceDataManager;

    public:
        InstanceGPU(InstanceDataManager* manager, DRE::U64 addressGPU, DRE::U32 id, InstanceFlags flags, MaterialsManager::MaterialGPU const& material);

        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform, DRE::U32 globalID, InstanceFlags instanceFlags);
        void ScheduleUpdate(glm::mat4 transform, glm::mat4 invTransform);
        void ScheduleUpdate(InstanceFlags flags, bool addFlags);
        void ScheduleUpdate(InstanceFlags flags);
        void ScheduleUpdate(MaterialsManager::MaterialGPU const& material);

        glm::mat4 const& GetTransform() const { return m_InstanceDataCPU.world_space; }
        MaterialsManager::MaterialGPU& GetMaterialGPU() { return m_MaterialGPU; }
        InstanceFlags GetFlags() const { return InstanceFlags{ m_InstanceDataCPU.globalID_instanceFlags.y }; }

    private:
        S_INSTANCE m_InstanceDataCPU;
        // for convinience is cached here, actual GPU ptr lives inside S_INSTANCE
        MaterialsManager::MaterialGPU m_MaterialGPU;
    };

public:
    InstanceDataManager(PersistentStorage* storage);

    InstanceGPU AllocateInstance(MaterialsManager::MaterialGPU const& material);
    void FreeInstance(InstanceGPU& instance);
private:
    friend class InstanceGPU;
};

}

