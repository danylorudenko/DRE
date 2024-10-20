#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\Container\Vector.hpp>
#include <foundation\memory\ElementAllocator.hpp>

#include <gfx\buffer\PersistentStorage.hpp>

#include <transforms.h>

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
class TransformsManager
    : public NonMovable
    , public NonCopyable
{
public:
    static constexpr std::uint32_t MAX_TRANSFORMS = 1024 * 32;

    class TransformWS
    {
        friend class TransformsManager;

    public:
        void ScheduleUpdate(glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& color, float flux, std::uint32_t type);

    private:
        TransformWS(TransformsManager* manager, std::uint32_t id);

        TransformsManager*  m_TransformsManager;
        std::uint32_t       m_id;
    };

public:
    TransformsManager(PersistentStorage* storage);

    TransformWS AllocateTransform();
    void FreeTransform(TransformWS& transform);

    void ScheduleTransformUpdate(std::uint32_t id, glm::vec3 const& position, glm::vec3 const& orientation, glm::vec3 const& scale);
    void ScheduleTransformUpdate(std::uint32_t id, glm::mat4 const& worldSpace);
    void UpdateGPUTransforms(VKW::Context& context);

    std::uint32_t GetTransformsCount() const;

    std::uint64_t GetBufferAddress() const;


private:
    PersistentStorage::Allocation m_PersistentAllocation;
    DRE::FreeListElementAllocator<MAX_TRANSFORMS> m_ElementAllocator;
    std::uint32_t m_TransformsCount;

    struct TransformUpdateEntry
    {
        std::uint32_t   id;
        S_TRANSFORM     payload;
    };
    DRE::InplaceVector<TransformUpdateEntry, 1024> m_TransformUpdateQueue;
};

}

