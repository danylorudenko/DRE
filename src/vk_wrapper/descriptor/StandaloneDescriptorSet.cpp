#include <vk_interface\descriptor\StandaloneDescriptorSet.hpp>

#include <foundation\Common.hpp>

#include <vk_interface\Helper.hpp>
#include <vk_interface\resources\Resource.hpp>
#include <vk_interface\ImportTable.hpp>
#include <vk_interface\LogicalDevice.hpp>
#include <vk_interface\descriptor\DescriptorManager.hpp>

namespace VKW
{

StandaloneDescriptorSet::WriteDescriptor::WriteDescriptor()
{
}

StandaloneDescriptorSet::Descriptor::Descriptor(DescriptorStage stages)
    : layoutDescriptor_{ stages }
{
}

void StandaloneDescriptorSet::WriteDescriptor::AddSamplers(VkSampler* samplers, std::uint8_t count, std::uint32_t binding)
{
    if (samplers != nullptr)
    {
        std::uint32_t const startInfo = imageInfos_.Size();

        for (std::uint8_t i = 0; i < count; i++)
        {
            VkDescriptorImageInfo& info = imageInfos_.EmplaceBack();
            info.sampler = samplers[i];
            info.imageView = VK_NULL_HANDLE;
            info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = count;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_SAMPLER);
        write.pImageInfo        = imageInfos_.Data() + startInfo;
        write.pBufferInfo       = nullptr;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::WriteDescriptor::AddStorageBuffer(BufferResource* buffer, std::uint32_t binding)
{
    if (buffer != nullptr)
    {
        VkDescriptorBufferInfo& info = bufferInfos_.EmplaceBack();
        info.buffer = buffer->handle_;
        info.offset = 0;
        info.range  = buffer->size_;

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = 1;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_STORAGE_BUFFER);
        write.pImageInfo        = nullptr;
        write.pBufferInfo       = &info;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::WriteDescriptor::AddStorageImage(ImageResourceView* view, std::uint32_t binding)
{
    if(view != nullptr)
    {
        VkDescriptorImageInfo& info = imageInfos_.EmplaceBack();
        info.sampler        = VK_NULL_HANDLE;
        info.imageView      = view->handle_;
        info.imageLayout    = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = 1;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_STORAGE_IMAGE);
        write.pImageInfo        = &info;
        write.pBufferInfo       = nullptr;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::WriteDescriptor::AddSampledImage(ImageResourceView* view, std::uint32_t binding)
{
    if (view != nullptr)
    {
        VkDescriptorImageInfo& info = imageInfos_.EmplaceBack();
        info.sampler        = VK_NULL_HANDLE;
        info.imageView      = view->handle_;
        info.imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = 1;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_TEXTURE);
        write.pImageInfo        = &info;
        write.pBufferInfo       = nullptr;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::WriteDescriptor::AddReadonlyImage(ImageResourceView* view, std::uint32_t binding)
{
    if(view != nullptr)
    {
        VkDescriptorImageInfo& info = imageInfos_.EmplaceBack();
        info.sampler        = VK_NULL_HANDLE;
        info.imageView      = view->handle_;
        info.imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = 1;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_TEXTURE);
        write.pImageInfo        = &info;
        write.pBufferInfo       = nullptr;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::WriteDescriptor::AddUniform(BufferResource* buffer, std::uint32_t offset, std::uint32_t size, std::uint32_t binding)
{
    if (buffer != nullptr)
    {
        VkDescriptorBufferInfo& info = bufferInfos_.EmplaceBack();
        info.buffer = buffer->handle_;
        info.offset = offset;
        info.range  = size;

        VkWriteDescriptorSet& write = writes_.EmplaceBack();
        write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext             = nullptr;
        write.dstSet            = VK_NULL_HANDLE;
        write.dstBinding        = binding;
        write.dstArrayElement   = 0;
        write.descriptorCount   = 1;
        write.descriptorType    = HELPER::DescriptorTypeToVK(DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        write.pImageInfo        = nullptr;
        write.pBufferInfo       = &info;
        write.pTexelBufferView  = nullptr;
    }
}

void StandaloneDescriptorSet::Descriptor::AddSamplers(VkSampler* samplers, std::uint8_t count, std::uint32_t binding)
{
    layoutDescriptor_.Add(DESCRIPTOR_TYPE_SAMPLER, binding, count);
    WriteDescriptor::AddSamplers(samplers, count, binding);
}

void StandaloneDescriptorSet::Descriptor::AddStorageBuffer(BufferResource* buffer, std::uint32_t binding)
{
    layoutDescriptor_.Add(DESCRIPTOR_TYPE_STORAGE_BUFFER, binding);
    WriteDescriptor::AddStorageBuffer(buffer, binding);
}

void StandaloneDescriptorSet::Descriptor::AddStorageImage(ImageResourceView* view, std::uint32_t binding)
{
    layoutDescriptor_.Add(DESCRIPTOR_TYPE_STORAGE_IMAGE, binding);
    WriteDescriptor::AddStorageImage(view, binding);
}

void StandaloneDescriptorSet::Descriptor::AddSampledImage(ImageResourceView* view, std::uint32_t binding)
{
    layoutDescriptor_.Add(DESCRIPTOR_TYPE_TEXTURE, binding);
    WriteDescriptor::AddSampledImage(view, binding);
}

void StandaloneDescriptorSet::Descriptor::AddUniform(BufferResource* buffer, std::uint32_t offset, std::uint32_t size, std::uint32_t binding)
{
    layoutDescriptor_.Add(DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding);
    WriteDescriptor::AddUniform(buffer, offset, size, binding);
}

void StandaloneDescriptorSet::WriteDescriptor::SetTargetSet(VkDescriptorSet target)
{
    for (std::uint16_t i = 0; i < writes_.Size(); i++)
    {
        writes_[i].dstSet = target;
    }
}

///////////////////////////////////////
StandaloneDescriptorSet::StandaloneDescriptorSet()
    : table_{ nullptr }
    , device_{ nullptr }
    , allocator_{ nullptr }
    , layout_{}
    , set_{ VK_NULL_HANDLE }
{}

StandaloneDescriptorSet::StandaloneDescriptorSet(ImportTable* table, LogicalDevice* device, Descriptor& descriptor, DescriptorManager* allocator)
    : table_{ table }
    , device_{ device }
    , allocator_{ allocator }
    , layout_{ table, device, descriptor.GetLayoutDescriptor() }
    , set_{ allocator->AllocateStandaloneSet(layout_) }
{
    Write(descriptor);
}

StandaloneDescriptorSet::StandaloneDescriptorSet(StandaloneDescriptorSet&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , allocator_{ nullptr }
    , layout_{}
    , set_{ VK_NULL_HANDLE }
{
    operator=(DRE_MOVE(rhs));
}
 
StandaloneDescriptorSet& StandaloneDescriptorSet::operator=(StandaloneDescriptorSet&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(allocator_);

    DRE_SWAP_MEMBER(layout_);

    DRE_SWAP_MEMBER(set_);

    return *this;
}

void StandaloneDescriptorSet::Write(WriteDescriptor& descriptor)
{
    descriptor.SetTargetSet(set_);

    table_->vkUpdateDescriptorSets(
        device_->Handle(),
        descriptor.WritesCount(),
        descriptor.Writes(),
        0, nullptr
    );
}

StandaloneDescriptorSet::~StandaloneDescriptorSet()
{
    if (set_ != VK_NULL_HANDLE)
    {
        allocator_->FreeStandaloneSet(set_);
    }
}

}
