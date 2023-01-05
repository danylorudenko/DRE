#pragma once

#include <foundation\Container\InplaceVector.hpp>
#include <vk_interface\descriptor\DescriptorLayout.hpp>

namespace VKW
{

struct BufferResource;
struct ImageResourceView;
class DescriptorManager;


class StandaloneDescriptorSet : public NonCopyable
{
public:
    class WriteDescriptor
        : public NonCopyable
        , public NonMovable
    {
    public:
        using WriteCollection = DRE::InplaceVector<VkWriteDescriptorSet, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;
        using ImageCollection = DRE::InplaceVector<VkDescriptorImageInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;
        using BufferCollection = DRE::InplaceVector<VkDescriptorBufferInfo, VKW::CONSTANTS::MAX_SET_LAYOUT_MEMBERS>;

        WriteDescriptor();

        void AddSamplers(VkSampler* samplers, std::uint8_t count, std::uint32_t binding);
        void AddStorageBuffer(BufferResource* buffer, std::uint32_t binding);
        void AddStorageImage(ImageResourceView* image, std::uint32_t binding);
        void AddSampledImage(ImageResourceView* image, std::uint32_t binding);
        void AddReadonlyImage(ImageResourceView* image, std::uint32_t binding);
        void AddUniform(BufferResource* buffer, std::uint32_t offset, std::uint32_t size, std::uint32_t binding);

        void SetTargetSet(VkDescriptorSet target);

        inline std::uint32_t                            WritesCount() const { return writes_.Size(); }
        inline VkWriteDescriptorSet const*              Writes() const { return writes_.Data(); }

    private:
        WriteCollection  writes_;
        ImageCollection  imageInfos_;
        BufferCollection bufferInfos_;
    };

    class Descriptor
        : public WriteDescriptor
    {
    public:
        Descriptor(DescriptorStage stages);

        void AddSamplers(VkSampler* samplers, std::uint8_t count, std::uint32_t binding);
        void AddStorageBuffer(BufferResource* buffer, std::uint32_t binding);
        void AddStorageImage(ImageResourceView* image, std::uint32_t binding);
        void AddSampledImage(ImageResourceView* image, std::uint32_t binding);
        void AddUniform(BufferResource* buffer, std::uint32_t offset, std::uint32_t size, std::uint32_t binding);

        inline DescriptorSetLayout::Descriptor const&   GetLayoutDescriptor() const { return layoutDescriptor_; }

    private:
        DescriptorSetLayout::Descriptor layoutDescriptor_;
    };


public:
    StandaloneDescriptorSet();
    StandaloneDescriptorSet(ImportTable* table, LogicalDevice* device, Descriptor& descriptor, DescriptorManager* allocator);

    StandaloneDescriptorSet(StandaloneDescriptorSet&& rhs);
    StandaloneDescriptorSet& operator=(StandaloneDescriptorSet&& rhs);

    ~StandaloneDescriptorSet();

    inline DescriptorSetLayout const& GetLayout() const { return layout_; }
    inline VkDescriptorSet GetSet() const { return set_; }

    void Write(WriteDescriptor& descriptor);

private:
    ImportTable*         table_;
    LogicalDevice*       device_;
    DescriptorManager*   allocator_;

    DescriptorSetLayout layout_;

    VkDescriptorSet     set_;
};

}

