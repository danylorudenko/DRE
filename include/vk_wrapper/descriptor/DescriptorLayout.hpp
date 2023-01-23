#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>
#include <limits>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\Common.hpp>
#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#define DEBUG_LAYOUT_MEMBERS

namespace VKW
{

class ImportTable;
class LogicalDevice;


///////////////////////////////////////////////

class DescriptorSetLayout : public NonCopyable
{
public:
    class Descriptor/* : public NonCopyable, public NonMovable*/
    {
    public:
        struct BindingDesc
        {
            DescriptorType          type_;
            std::uint32_t           binding_;
            VKW::DescriptorStage    stage_;
            std::uint32_t           count_ : 16;
            std::uint32_t           variableCount_ : 1;
            std::uint32_t           updateAfterBind_ : 1;
        };


    public:
        Descriptor(/*DescriptorStage stages*/);

        // returns binding
        std::uint16_t Add(DescriptorType type, std::uint32_t binding, VKW::DescriptorStage stage, std::uint32_t count = 1);
        std::uint16_t AddVariableCount(DescriptorType type, std::uint32_t binding, VKW::DescriptorStage stage, std::uint32_t count);

        std::uint16_t                   GetCount() const;
        BindingDesc const&              GetMember(std::uint16_t i) const;
        DescriptorStage                 GetStagesMask() const;

    private:
        std::uint16_t           count_;
        BindingDesc             members_[CONSTANTS::MAX_SET_LAYOUT_MEMBERS];
        DescriptorStage         stagesMask_;

        bool isClosed_;

    };

public:
    DescriptorSetLayout();
    DescriptorSetLayout(ImportTable* table, LogicalDevice* device, Descriptor const& descriptor);

    DescriptorSetLayout(DescriptorSetLayout&& rhs);
    DescriptorSetLayout& operator=(DescriptorSetLayout&& rhs);

    ~DescriptorSetLayout();

    inline VkDescriptorSetLayout    GetHandle() const { return handle_; };

private:
    ImportTable*            table_;
    LogicalDevice*          device_;

    VkDescriptorSetLayout   handle_;

#ifdef DEBUG_LAYOUT_MEMBERS
    Descriptor descriptor_;
#endif

};

/////////////////////////////////////////////////////

class PipelineLayout : public NonCopyable
{
public:
    class Descriptor/* : public NonMovable, public NonCopyable*/
    {
    public:
        Descriptor();

        void Add(DescriptorSetLayout const* layout);
        void AddPushConstant(std::uint8_t size, DescriptorStage stages);
        void Set(std::uint16_t setId, DescriptorSetLayout* layout);

        std::uint16_t                           GetSetCount() const;
        DescriptorSetLayout const*              GetLayout(std::uint16_t i) const;

        std::uint8_t                            GetPushConstantsCount() const;
        VkPushConstantRange const&              GetPushConstant(std::uint8_t i) const;


    private:
        std::uint16_t               count_;
        DescriptorSetLayout const*  members_[CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS];

        std::uint8_t            pushConstantCount_;
        std::uint8_t            pushConstantPointer_;
        VkPushConstantRange     pushConstantRanges_[CONSTANTS::MAX_PUSH_CONSTANTS];
    };


public:
    PipelineLayout();
    PipelineLayout(ImportTable* table, LogicalDevice* device, Descriptor const& descriptor);

    PipelineLayout(PipelineLayout&& rhs);
    PipelineLayout& operator=(PipelineLayout&& rhs);

    ~PipelineLayout();


    inline VkPipelineLayout             GetHandle() const { return handle_; }
    inline std::uint16_t                GetMemberCount() const { return memberCount_; }
    inline DescriptorSetLayout const*   GetMember(std::uint8_t i) const { return members_[i]; }

private:
    ImportTable*        table_;
    LogicalDevice*      device_;

    VkPipelineLayout    handle_;
    std::uint16_t       memberCount_;

    DRE::InplaceVector<DescriptorSetLayout const*, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS> members_;

#ifdef DEBUG_LAYOUT_MEMBERS
    Descriptor descriptor_;
#endif

};

}