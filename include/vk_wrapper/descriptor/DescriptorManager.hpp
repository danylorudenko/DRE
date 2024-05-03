#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\container\InplaceVector.hpp>

#include <vulkan\vulkan.h>

#include <foundation\memory\ElementAllocator.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>
#include <vk_wrapper\descriptor\DescriptorLayout.hpp>

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
    class WriteDesc
        : public NonCopyable
        , public NonMovable
    {
    public:
        using WriteCollection = DRE::InplaceVector<VkWriteDescriptorSet, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;
        using ImageCollection = DRE::InplaceVector<VkDescriptorImageInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;
        using BufferCollection = DRE::InplaceVector<VkDescriptorBufferInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;

        WriteDesc();

        void AddSamplers(VkSampler* samplers, std::uint8_t count, std::uint32_t binding);
        void AddStorageBuffer(BufferResource* buffer, std::uint32_t binding);
        void AddStorageImage(ImageResourceView* image, std::uint32_t binding);
        void AddSampledImage(ImageResourceView* image, std::uint32_t binding);
        void AddUniform(BufferResource* buffer, std::uint32_t offset, std::uint32_t size, std::uint32_t binding);
        
        void AddTarget(VkDescriptorSet targetSet);

        inline std::uint32_t                WritesCount() const { return writes_.Size(); }
        inline VkWriteDescriptorSet const*  Writes() const { return writes_.Data(); }

    private:
        WriteCollection  writes_;
        ImageCollection  imageInfos_;
        BufferCollection bufferInfos_;
    };

public:
    DescriptorManager(ImportTable* table, LogicalDevice* device);

    DescriptorManager(DescriptorManager&& rhs);
    DescriptorManager& operator=(DescriptorManager&& rhs);

    ~DescriptorManager();

    void                        AllocateDefaultDescriptors(std::uint8_t globalBuffersCount, BufferResource** globalUniformBuffers, BufferResource* persistentStorageBuffer);

    TextureDescriptorIndex      AllocateTextureDescriptor(ImageResourceView const* view = nullptr);
    void                        FreeTextureDescriptor(TextureDescriptorIndex& handle);

    DescriptorSet               AllocateStandaloneSet(DescriptorSetLayout const& layout);
    void                        FreeStandaloneSet(DescriptorSet& set);
    VkDescriptorPool            GetStandaloneDescriptorPool(); // needed for ImGui backend

    std::uint32_t               GetGlobalSetLayoutsCount() const { return 3u; }
    DescriptorSetLayout const*  GetGlobalSetLayouts() const { return globalSetLayouts_; }
    DescriptorSetLayout&        GetGlobalSetLayout(std::uint32_t i) { return globalSetLayouts_[i]; }
    PipelineLayout*             GetGlobalPipelineLayout() { return &globalPipelineLayout_; }

    DescriptorSet               GetGlobalSampler() const { return DescriptorSet{ globalSampler_, globalSetLayouts_ + 0 }; }
    DescriptorSet               GetGlobalTexturesSet() const { return DescriptorSet{ globalTexturesSet_, globalSetLayouts_ + 1 }; }
    DescriptorSet               GetGlobalUniformSet(std::uint8_t bufferingID) { return DescriptorSet{ globalUniformSets_[bufferingID], globalSetLayouts_ + 2 }; }

    VkSampler                   GetDefaultSampler(SamplerType type) const;

    void                        WriteDescriptorSet(DescriptorSet set, WriteDesc& desc);

private:
    void CreateGlobalDescriptorLayouts();
    void WriteTextureDescriptor(VkDescriptorSet set, std::uint16_t descriptorID, ImageResourceView const* view);

private:
    ImportTable* table_;
    LogicalDevice* device_;

private:
    
    DRE::FreeListElementAllocator<VKW::CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE> dynamicTextureHeap_;

    VkSampler                   defaultSamplers_[(int)SAMPLER_TYPE_MAX];

    DescriptorSetLayout         globalSetLayouts_[3];
    PipelineLayout              globalPipelineLayout_;

    VkDescriptorPool            globalSetPool_;
    VkDescriptorPool            globalTexturesPool_;

    VkDescriptorSet             globalSampler_;
    VkDescriptorSet             globalTexturesSet_;
    VkDescriptorSet             globalUniformSets_[VKW::CONSTANTS::FRAMES_BUFFERING];

    VkDescriptorPool            standalonePool_;

};

}
