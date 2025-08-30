#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\OffsetAllocator.hpp>
#include <gfx\renderer\GPUInstanceAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

#include <common\materials.h>

#include <glm\vec4.hpp>

namespace VKW
{
class Context;
}

namespace GFX
{

class PersistentStorage;

class MaterialsManager
    : public GPUInstanceAllocator<S_MATERIAL, 1024 * 32, 1024>
{
public:
    static constexpr std::uint32_t MAX_MATERIALS = 1024 * 32;

    using Base = GPUInstanceAllocator<S_MATERIAL, MAX_MATERIALS, 1024>;

    class MaterialGPU : public Base::Payload
    {
        friend class MaterialsManager;

    public:
        MaterialGPU(MaterialsManager* manager, DRE::U64 addressGPU, DRE::U32 id);

        void ScheduleUpdate(glm::ivec4 textureIDs, glm::ivec4 auxTextureIDs, MaterialFlags flags);
        void ScheduleUpdateTextures(glm::ivec4 textureIDs);
        void ScheduleUpdateAuxTextures(glm::ivec4 textureIDs);
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

