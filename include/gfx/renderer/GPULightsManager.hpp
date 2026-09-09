#pragma once

#include <foundation\Common.hpp>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>
#include <gfx\renderer\GPUInstanceAllocator.hpp>

#include <common\lighting\lights.slang>

#include <glm\vec3.hpp>

namespace VKW
{
class Context;
class DescriptorManager;
}

namespace GFX
{

class PersistentStorage;


constexpr DRE::U32 MAX_GPU_LIGHTS           = 1024;
constexpr DRE::U32 MAX_GPU_LIGHTS_QUEUE     = 64;

using LightsManagerBase = GPUInstanceAllocator<S_LIGHT, MAX_GPU_LIGHTS, MAX_GPU_LIGHTS_QUEUE>;


/////////////////////////////
class LightsManager
    : public LightsManagerBase
{
public:
    using Base = LightsManagerBase;

    class LightGPU : public Base::Payload
    {
        friend class LightsManager;

    public:
        LightGPU(LightsManager* manager, DRE::U64 addressGPU, DRE::U32 id);
        void ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float intensity, DRE::U32 type);
    };

public:
    LightsManager(PersistentStorage* storage);

    LightGPU    AllocateLight();
    void        FreeLight(LightGPU& light);
};

}

