#pragma once

#include <vulkan\vulkan.h>

#include <foundation\Common.hpp>
#include <foundation\class_features\NonCopyable.hpp>

namespace VKW
{

////////////////////////////////////////////
enum DescriptorStageBits : std::uint16_t
{
    DESCRIPTOR_STAGE_NONE        = 0,
    DESCRIPTOR_STAGE_COMPUTE     = 1,
    DESCRIPTOR_STAGE_VERTEX      = 1 << 1,
    DESCRIPTOR_STAGE_FRAGMENT    = 1 << 2,
    DESCRIPTOR_STAGE_RENDERING   = DESCRIPTOR_STAGE_VERTEX | DESCRIPTOR_STAGE_FRAGMENT,
    DESCRIPTOR_STAGE_ALL         = DESCRIPTOR_STAGE_COMPUTE | DESCRIPTOR_STAGE_RENDERING
};

using DescriptorStage = std::uint16_t;

////////////////////////////////////////////
enum DescriptorTypeBits
{
    DESCRIPTOR_TYPE_NONE                    = 0,
    DESCRIPTOR_TYPE_TEXTURE                 = 1 << 0,
    DESCRIPTOR_TYPE_SAMPLER                 = 1 << 1,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER          = 1 << 2,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC  = 1 << 3,
    DESCRIPTOR_TYPE_STORAGE_IMAGE           = 1 << 4,
    DESCRIPTOR_TYPE_STORAGE_BUFFER          = 1 << 5,
    DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC  = 1 << 6,
};
using DescriptorType = std::uint16_t;

inline bool IsTextureDescriptor(DescriptorType type)
{
    switch (type)
    {
    case DESCRIPTOR_TYPE_TEXTURE:
    case DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return true;
    case DESCRIPTOR_TYPE_SAMPLER:
    case DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    case DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return false;
    default:
        DRE_ASSERT(false, "Unsupported VKW::DescriptorType.");
        return false;
    }
}


///////////////////////////////
struct GlobalDescriptorHandle
{
    inline GlobalDescriptorHandle(std::uint32_t id = 0, std::uint32_t count = 0)
        : id_{ id }, count_{ count }
    {}

    inline bool IsValid() const { return count_ > 0; }

    std::uint32_t   id_;
    std::uint32_t   count_;
};

///////////////////////////////
// 
class DescriptorSetLayout;
// simple wrapper, doesn't own anything
class DescriptorSet
{
public:
    DescriptorSet()
        : set_{ VK_NULL_HANDLE }
        , layout_{ nullptr }
    {}

    DescriptorSet(VkDescriptorSet set, DescriptorSetLayout const* layout)
        : set_{ set }
        , layout_{ layout }
    {}

    DescriptorSet(DescriptorSet const& rhs)
        : set_{ rhs.set_ }
        , layout_{ rhs.layout_ }
    {}

    DescriptorSet(DescriptorSet&& rhs)
        : set_{ rhs.set_ }
        , layout_{ rhs.layout_ }
    {
        rhs.set_ = VK_NULL_HANDLE;
        rhs.layout_ = nullptr;
    }

    DescriptorSet& operator=(DescriptorSet const& rhs)
    {
        set_ = rhs.set_;
        layout_ = rhs.layout_;

        return *this;
    }

    DescriptorSet& operator=(DescriptorSet&& rhs)
    {
        set_ = rhs.set_;        rhs.set_ = VK_NULL_HANDLE;
        layout_ = rhs.layout_;  rhs.layout_ = nullptr;
    
        return *this;
    }

    inline VkDescriptorSet GetHandle() const { return set_; }
    inline DescriptorSetLayout const* GetLayout() const { return layout_; }

private:
    VkDescriptorSet set_;
    DescriptorSetLayout const* layout_;
};

}

