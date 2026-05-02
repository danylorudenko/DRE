#pragma once

#include <foundation\Common.hpp>

#include <gfx\renderer\GPUInstanceAllocator.hpp>
#include <glm\vec4.hpp>

#include <common\materials.h>

namespace VKW
{
class Context;
}

namespace GFX
{

class PersistentStorage;


constexpr DRE::U32 MAX_GPU_MATERIALS        = 1024;
constexpr DRE::U32 MAX_GPU_MATERIALS_QUEUE  = 64;

using MaterialsManagerBase = GPUInstanceAllocator<S_MATERIAL, MAX_GPU_MATERIALS, MAX_GPU_MATERIALS_QUEUE>;

//////////////////////////
class MaterialsManager
    : public MaterialsManagerBase
{
public:
    using Base = MaterialsManagerBase;

    class MaterialGPU : public Base::Payload
    {
        friend class MaterialsManager;

    public:
        MaterialGPU(MaterialsManager* manager, DRE::U64 addressGPU, DRE::U32 id);

        void ScheduleUpdate(S_MATERIAL materialData);
        void ScheduleUpdateTextures0(glm::ivec4 textureIDs);
        void ScheduleUpdateTextures1(glm::ivec4 textureIDs);
        void ScheduleUpdate(MaterialFlags flags, bool addFlags);
        void ScheduleUpdate(MaterialFlags flags);

    private:
        S_MATERIAL m_MaterialDataCPU;
    };

public:
    MaterialsManager(PersistentStorage* storage);

    MaterialGPU AllocateMaterial();
    void FreeMaterial(MaterialGPU& material);
};

} // namespace GFX

