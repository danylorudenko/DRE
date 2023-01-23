#include <vk_wrapper\descriptor\DescriptorLayout.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\Helper.hpp>

#include <utility>
#include <cassert>
#include <algorithm>

namespace VKW
{

DescriptorSetLayout::Descriptor::Descriptor(/*DescriptorStage stages*/)
    : count_{ 0 }
    , members_{ 0 }
    , stagesMask_{ /*stages*/VKW::DESCRIPTOR_STAGE_NONE }
    , isClosed_{ false }
{}

std::uint16_t DescriptorSetLayout::Descriptor::Add(DescriptorType type, std::uint32_t binding, VKW::DescriptorStage stage, std::uint32_t count)
{
    DRE_ASSERT(!isClosed_, "Attempt to modify closed descriptor layout decorator.");
    DRE_ASSERT((count_ + 1 < CONSTANTS::MAX_SET_LAYOUT_MEMBERS), "Maximum count of descriptor bindings in set is MAX_SET_LAYOUT_MEMBERS");
    
    members_[count_].type_ = type;
    members_[count_].binding_ = binding;
    members_[count_].stage_ = stage;
    members_[count_].count_ = count;
    members_[count_].variableCount_ = 0;
    members_[count_].updateAfterBind_ = 0;

    stagesMask_ |= stage;

    return count_++;
}

std::uint16_t DescriptorSetLayout::Descriptor::AddVariableCount(DescriptorType type, std::uint32_t binding, VKW::DescriptorStage stage, std::uint32_t count)
{
    DRE_ASSERT(!isClosed_, "Attempt to modify closed descriptor layout decorator.");
    DRE_ASSERT((count_ + 1 < CONSTANTS::MAX_SET_LAYOUT_MEMBERS), "Maximum count of descriptor bindings in set is MAX_SET_LAYOUT_MEMBERS");
    DRE_ASSERT(type != DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "Dynamic buffers can't be bindless");

    members_[count_].type_ = type;
    members_[count_].binding_ = binding;
    members_[count_].stage_ = stage;
    members_[count_].count_ = count;
    members_[count_].variableCount_ = 1;
    members_[count_].updateAfterBind_ = 1;

    stagesMask_ |= stage;

    isClosed_ = true;
    return count_++;
}

std::uint16_t DescriptorSetLayout::Descriptor::GetCount() const
{
    return count_;
}

DescriptorSetLayout::Descriptor::BindingDesc const& DescriptorSetLayout::Descriptor::GetMember(std::uint16_t i) const
{
    DRE_ASSERT(i < count_, "DescriptorSetLayout member access out of bounds.");
    return members_[i];
}

DescriptorStage DescriptorSetLayout::Descriptor::GetStagesMask() const
{
    return stagesMask_;
}

DescriptorSetLayout::DescriptorSetLayout()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , descriptor_{ /*VKW::DESCRIPTOR_STAGE_NONE*/ }
{
}

DescriptorSetLayout::DescriptorSetLayout(ImportTable* table, LogicalDevice* device, Descriptor const& descriptor)
    : table_{ table }
    , device_{ device }
    , handle_{ VK_NULL_HANDLE }
    , descriptor_{ descriptor }
{
    VkShaderStageFlags stageFlags = HELPER::DescriptorStageToVK(descriptor.GetStagesMask());

    VkDescriptorSetLayoutBinding bindings[CONSTANTS::MAX_SET_LAYOUT_MEMBERS];
    VkDescriptorBindingFlags bindingFlags[CONSTANTS::MAX_SET_LAYOUT_MEMBERS];
    VkFlags aggregateBindingFlags = VK_FLAGS_NONE;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.pNext = nullptr;
    bindingFlagsInfo.bindingCount = descriptor.GetCount();
    bindingFlagsInfo.pBindingFlags = bindingFlags;

    for (std::uint32_t i = 0u; i < descriptor.GetCount(); ++i) {
        bindings[i].binding             = descriptor.GetMember(i).binding_;
        bindings[i].descriptorType      = HELPER::DescriptorTypeToVK(descriptor.GetMember(i).type_);
        bindings[i].descriptorCount     = descriptor.GetMember(i).count_;
        bindings[i].stageFlags          = stageFlags;
        bindings[i].pImmutableSamplers  = nullptr;

        bindingFlags[i] = VK_FLAGS_NONE;
        if (descriptor.GetMember(i).variableCount_)
            aggregateBindingFlags = bindingFlags[i] |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

        if (descriptor.GetMember(i).updateAfterBind_)
            aggregateBindingFlags = bindingFlags[i] |=
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    }

    VkDescriptorSetLayoutCreateInfo setInfo;
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setInfo.pNext = &bindingFlagsInfo;
    setInfo.bindingCount = descriptor.GetCount();
    setInfo.pBindings = bindings;
    setInfo.flags = VK_FLAGS_NONE;

    if (aggregateBindingFlags & (VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT))
    {
        setInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    }

    VK_ASSERT(table_->vkCreateDescriptorSetLayout(device_->Handle(), &setInfo, nullptr, &handle_));
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , descriptor_{ /*VKW::DESCRIPTOR_STAGE_NONE*/ }
{
    operator=(DRE_MOVE(rhs));
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(descriptor_);

    return *this;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyDescriptorSetLayout(device_->Handle(), handle_, nullptr);
    }
}


//////////////////////////////////////////////////

PipelineLayout::Descriptor::Descriptor()
    : count_{ 0 }
    , members_{ 0 }
    , pushConstantCount_{ 0 }
    , pushConstantPointer_{ 0 }
{}

void PipelineLayout::Descriptor::Add(DescriptorSetLayout const* layout)
{
    DRE_ASSERT(count_ + 1 < CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS, "Maximum count of descriptor sets in pipeline layout is MAX_PIPELINE_LAYOUT_MEMBERS.");
    members_[count_++] = layout;
}

void PipelineLayout::Descriptor::AddPushConstant(std::uint8_t size, DescriptorStage stages)
{
    DRE_ASSERT(pushConstantCount_ < CONSTANTS::MAX_PUSH_CONSTANTS, "Push constant ranges overflow");
    DRE_ASSERT(pushConstantPointer_ + size <= 128, "Push constant max size overflow.");


    VkPushConstantRange& range = pushConstantRanges_[pushConstantCount_++];
    range.stageFlags = HELPER::DescriptorStageToVK(stages);
    range.offset = pushConstantPointer_;
    range.size = size;

    pushConstantPointer_ += size;
}

void PipelineLayout::Descriptor::Set(std::uint16_t setId, DescriptorSetLayout* layout)
{
    DRE_ASSERT(setId - 1 < CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS, "Maximum count of descriptor sets in pipeline layout is MAX_PIPELINE_LAYOUT_MEMBERS.");
    members_[setId] = layout;
}


std::uint16_t PipelineLayout::Descriptor::GetSetCount() const
{
    return count_;
}

DescriptorSetLayout const* PipelineLayout::Descriptor::GetLayout(std::uint16_t i) const
{
    DRE_ASSERT(i < count_, "Pipeline layout member access out of bounds.");
    return members_[i];
}

std::uint8_t PipelineLayout::Descriptor::GetPushConstantsCount() const
{
    return pushConstantCount_;
}

VkPushConstantRange const& PipelineLayout::Descriptor::GetPushConstant(std::uint8_t i) const
{
    return pushConstantRanges_[i];
}


PipelineLayout::PipelineLayout()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , memberCount_{ 0 }
{
}

PipelineLayout::PipelineLayout(ImportTable* table, LogicalDevice* device, Descriptor const& descriptor)
    : table_{ table }
    , device_{ device }
    , handle_{ VK_NULL_HANDLE }
    , memberCount_{ 0 }
    , descriptor_{ descriptor }
{
    VkDescriptorSetLayout layouts[CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS];

    for (std::uint16_t i = 0u; i < descriptor.GetSetCount(); ++i) {
        DescriptorSetLayout const* layout = members_.EmplaceBack(descriptor.GetLayout(i));
        layouts[i] = layout != nullptr ? layout->GetHandle() : VK_NULL_HANDLE;
    }

    VkPushConstantRange pushRanges[CONSTANTS::MAX_PUSH_CONSTANTS];

    for (std::uint8_t i = 0; i < descriptor.GetPushConstantsCount(); i++)
    {
        pushRanges[i] = descriptor.GetPushConstant(i);
    }

    VkPipelineLayoutCreateInfo cInfo;
    cInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    cInfo.pNext = nullptr;
    cInfo.flags = VK_FLAGS_NONE;
    cInfo.pushConstantRangeCount = descriptor.GetPushConstantsCount();
    cInfo.pPushConstantRanges = pushRanges  ;
    cInfo.setLayoutCount = descriptor.GetSetCount();
    cInfo.pSetLayouts = layouts;

    VK_ASSERT(table_->vkCreatePipelineLayout(device_->Handle(), &cInfo, nullptr, &handle_));
    memberCount_ = descriptor.GetSetCount();

}

PipelineLayout::PipelineLayout(PipelineLayout&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
{
    operator=(DRE_MOVE(rhs));
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(memberCount_);
    DRE_SWAP_MEMBER(descriptor_);

    return *this;
}

PipelineLayout::~PipelineLayout()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyPipelineLayout(device_->Handle(), handle_, nullptr);
    }
}

}