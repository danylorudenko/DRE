#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <class_features\NonCopyable.hpp>

#include <foundation\Container\InplaceVector.hpp>

#include <vk_interface\Constant.hpp>
#include <vk_interface\resources\Resource.hpp>

namespace VKW
{

class ImportTable;
class LogicalDevice;
class CommandList;
class Framebuffer;

enum AttachmentOp
{
    ATTACHMENT_OP_LOAD,
    ATTACHMENT_OP_STORE,
    ATTACHMENT_OP_DONT_CARE
};



class RenderPass : public NonCopyable
{
public:
    static std::uint8_t constexpr MAX_REFERENCES = 10;
    static std::uint8_t constexpr MAX_SUBPASSES = 2;

public:
    struct Descriptor
    {
        void AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, float clearColor[4]);
        void AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, std::int32_t clearColor[4]);
        void AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, std::uint32_t clearColor[4]);
        void AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, float depthValue, std::uint32_t stencilValue);
        void AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat);

        void BeginGraphicsSubpass();
        void EndSubpass();

        void AddSubpassInput(std::uint8_t attachment, VkImageLayout layout);
        void AddSubpassOutput(std::uint8_t attachment, VkImageLayout layout);
        void AddSubpassDepthStencil(std::uint8_t attachment, VkImageLayout layout);

        VkRenderPassCreateInfo const& CompileRenderPassCreateInfo();

        VkRenderPassCreateInfo  createInfo_;

        DRE::InplaceVector<VkAttachmentDescription, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>  attachmentDescriptions_;
        DRE::InplaceVector<VkClearValue, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>             attachmentClears_;
        DRE::InplaceVector<VkAttachmentReference, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>    inputReferences_;
        DRE::InplaceVector<VkAttachmentReference, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>    colorReferences_;
        DRE::InplaceVector<VkSubpassDescription, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>     subpassInfos_;
        DRE::InplaceVector<VkSubpassDependency, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS>      subpassDependencies_;
        VkAttachmentReference                                        depthStencilReference_;

        std::uint8_t inputReferenceBeginPointer_ = 0;
        std::uint8_t inputReferencePerSubpassCount_ = 0;

        std::uint8_t colorReferenceBeginPointer_ = 0;
        std::uint8_t colorReferencePerSubpassCount_ = 0;
    };
    
public:
    RenderPass();
    RenderPass(ImportTable* table, LogicalDevice* device, Descriptor& descriptor);

    RenderPass(RenderPass&& rhs);
    RenderPass& operator=(RenderPass&& rhs);

    ~RenderPass();

    inline VkRenderPass GetHandle() const { return handle_; }
    inline std::uint8_t GetAttachmentCount() const { return attachmentCount_; }
    inline std::uint8_t GetSubpassCount() const { return subpassCount_; }

    void Begin(CommandList* commandList, Framebuffer* framebuffer);
    void NextSubpass(CommandList* commandList);
    void End(CommandList* commandList);


private:
    ImportTable*    table_;
    LogicalDevice*         device_;

    VkRenderPass handle_;

    std::uint8_t attachmentCount_;
    std::uint8_t subpassCount_;

    DRE::InplaceVector<VkClearValue, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> clearValues_;

};

}