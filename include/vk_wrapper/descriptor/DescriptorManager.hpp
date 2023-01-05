#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

#include <class_features\NonCopyable.hpp>

#include <vulkan\vulkan.h>

#include <memory\ElementAllocator.hpp>

#include <vk_interface\Constant.hpp>
#include <vk_interface\ImportTable.hpp>
#include <vk_interface\LogicalDevice.hpp>
#include <vk_interface\Tools.hpp>
#include <vk_interface\descriptor\Descriptor.hpp>
#include <vk_interface\descriptor\DescriptorLayout.hpp>

/* 
* This allocator implements "bind once, use always pattern" for
* first descriptor layout for the whole application.
* This also must be enforced in higher abstraction levels of the renderer.
* 
* 
* The allocator also provides a descriptor pool with FREE flags for StandaloneDescriptorSets.
* 
*/

namespace VKW
{

///////////////////////////////
enum SamplerType
{
    SAMPLER_TYPE_NEAREST_REPEAT,
    SAMPLER_TYPE_LINEAR_REPEAT,
    SAMPLER_TYPE_LINEAR_CLAMP,
    SAMPLER_TYPE_ANISOTROPIC,
    SAMPLER_TYPE_MAX
};

///////////////////////////////
struct HWDescriptorHandle
{
    inline HWDescriptorHandle(VkDescriptorSet handle)
        : handle_{ handle }
    {}

    VkDescriptorSet handle_;
};

/////////////////////////////////////////

class LogicalDevice;
class ImportTable;
struct ImageResourceView;
struct BufferResource;

///////////////////////////////
class DescriptorManager
    : public NonCopyable
{
public:
    DescriptorManager(ImportTable* table, LogicalDevice* device);

    DescriptorManager(DescriptorManager&& rhs);
    DescriptorManager& operator=(DescriptorManager&& rhs);

    ~DescriptorManager();

    void                    AllocateDefaultDescriptors(std::uint8_t globalBuffersCount, BufferResource** globalUniformBuffers, BufferResource* persistentStorageBuffer);

    GlobalDescriptorHandle  AllocateTextureDescriptor(ImageResourceView const* view = nullptr);
    void                    WriteTextureDescriptor(VkDescriptorSet set, std::uint16_t descriptorID, ImageResourceView const* view);
    void                    FreeTextureDescriptor(GlobalDescriptorHandle& handle);

    VkDescriptorSet         AllocateStandaloneSet(DescriptorSetLayout const& layout);
    void                    FreeStandaloneSet(VkDescriptorSet set);

    std::uint32_t               GetGlobalSetLayoutsCount() const { return (std::uint32_t)3; }
    DescriptorSetLayout const*  GetGlobalSetLayouts() const { return globalSetLayouts_; }
    DescriptorSetLayout&        GetGlobalSetLayout(std::uint32_t i) { return globalSetLayouts_[i]; }
    PipelineLayout*             GetGlobalPipelineLayout() { return &globalPipelineLayout_; }

    HWDescriptorHandle          GetGlobalSampler_StorageSet() const { return HWDescriptorHandle{ globalSampler_StorageSet_ }; }
    HWDescriptorHandle          GetGlobalTexturesSet() const { return HWDescriptorHandle{ globalTexturesSet_}; }
    HWDescriptorHandle          GetGlobalUniformSet(std::uint8_t bufferingID) { return HWDescriptorHandle{ globalUniformSets_[bufferingID] }; }

    VkSampler               GetDefaultSampler(SamplerType type) const;

private:
    void CreateGlobalDescriptorLayouts();

private:
    ImportTable* table_;
    LogicalDevice* device_;

private:
    /*
    template<typename TAllocator>
    struct DescriptorHeap
    {
        VkDescriptorSetLayout   layout_         = VK_NULL_HANDLE;
        VkDescriptorPool        pool_           = VK_NULL_HANDLE;
        VkDescriptorSet         set_            = VK_NULL_HANDLE;
        VkDescriptorType        descriptorType_ = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        HeapType                heapType_       = HEAP_TYPE_MAX;
        std::uint32_t           size_   = 0;
        bool                    isReset_= false;

        TAllocator              allocator_;

        void Init(ImportTable* table, LogicalDevice* device, HeapType heapType, VkDescriptorType descriptorType, std::uint32_t size);
        void Reset(ImportTable* table, LogicalDevice* device);
        void Destroy(ImportTable* table, LogicalDevice* device);

        DescriptorHandle Allocate(std::uint16_t count = 1);
        void             Free(DescriptorHandle& handle);
    };
    */

    DRE::FreeListElementAllocator<VKW::CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE>/*>*/ dynamicTextureHeap_;

    VkSampler                   defaultSamplers_[(int)SAMPLER_TYPE_MAX];

    DescriptorSetLayout         globalSetLayouts_[3];
    PipelineLayout              globalPipelineLayout_;

    VkDescriptorPool            globalSetPool_;
    VkDescriptorPool            globalTexturesPool_;

    VkDescriptorSet             globalSampler_StorageSet_;
    VkDescriptorSet             globalTexturesSet_;
    VkDescriptorSet             globalUniformSets_[VKW::CONSTANTS::FRAMES_BUFFERING];

    VkDescriptorPool            standalonePool_;

};

/*
template<typename TAllocator>
void DescriptorAllocator::DescriptorHeap<TAllocator>::Init(ImportTable* table, LogicalDevice* device, HeapType heapType, VkDescriptorType descriptorType, std::uint32_t count)
{
    assert(set_ == VK_NULL_HANDLE);

    descriptorType_ = descriptorType;
    heapType_ = heapType;
    size_ = count;
    isReset_ = true;

    
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorType = descriptorType;
    binding.descriptorCount = count;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorBindingFlags flag =
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |         // can write descriptor after bound
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | // unknown size, must be last in set
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |           // not all descriptors must be valid on the whole set (unless they're used)
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;// can update descriptors in set that is in execution (unless that particular descriptor is used right now)

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags;
    bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlags.pNext = nullptr;
    bindingFlags.bindingCount = 1;
    bindingFlags.pBindingFlags = &flag;

    VkDescriptorSetLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = &bindingFlags;
    createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &binding;

    VK_ASSERT(table->vkCreateDescriptorSetLayout(device->Handle(), &createInfo, nullptr, &layout_));

    ///////////////////

    VkDescriptorPoolSize poolSize;
    poolSize.type = descriptorType;
    poolSize.descriptorCount = count;

    VkDescriptorPoolCreateInfo poolInfo;
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    VK_ASSERT(table->vkCreateDescriptorPool(device->Handle(), &poolInfo, nullptr, &pool_));

    ///////////////////

    std::uint32_t countsData = count;
    VkDescriptorSetVariableDescriptorCountAllocateInfo counts;
    counts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    counts.pNext = nullptr;
    counts.descriptorSetCount = 1;
    counts.pDescriptorCounts = &countsData;


    VkDescriptorSetAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = &counts;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout_;

    VK_ASSERT(table->vkAllocateDescriptorSets(device->Handle(), &allocInfo, &set_));
}

template<typename TAllocator>
void DescriptorAllocator::DescriptorHeap<TAllocator>::Reset(ImportTable* table, LogicalDevice* device)
{
    allocator_.Reset();
    VK_ASSERT(table->vkResetDescriptorPool(device->Handle(), pool_, VK_FLAGS_NONE));
}

template<typename TAllocator>
void DescriptorAllocator::DescriptorHeap<TAllocator>::Destroy(ImportTable* table, LogicalDevice* device)
{
    VkDevice deviceHandle = device->Handle();

    table->vkDestroyDescriptorPool(deviceHandle, pool_, nullptr);           pool_ = VK_NULL_HANDLE;
    table->vkDestroyDescriptorSetLayout(deviceHandle, layout_, nullptr);    layout_ = VK_NULL_HANDLE;
}

template<typename TAllocator>
DescriptorHandle DescriptorAllocator::DescriptorHeap<TAllocator>::Allocate(std::uint16_t count)
{
    std::uint16_t const id = allocator_.Allocate(count);
    return DescriptorHandle{ id, count, heapType_ };
}

template<typename TAllocator>
void DescriptorAllocator::DescriptorHeap<TAllocator>::Free(DescriptorHandle& handle)
{
    allocator_.Free(handle.id_);
}

*/

}
