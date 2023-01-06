#include <vk_wrapper\pipeline\RenderPass.hpp>

#include <foundation\Common.hpp>
#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>
#include <vk_wrapper\queue\Queue.hpp>
#include <vk_wrapper\Tools.hpp>

namespace VKW
{

void RenderPass::Descriptor::AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, float clearColor[4])
{
    VkAttachmentDescription& desc = attachmentDescriptions_.EmplaceBack();

    desc.flags          = VK_FLAGS_NONE;
    desc.format         = Format2VK(imageFormat);
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = initialLayout;
    desc.finalLayout    = finalLayout;

    VkClearValue& clearValue = attachmentClears_.EmplaceBack();
    clearValue.color.float32[0] = clearColor[0];
    clearValue.color.float32[1] = clearColor[1];
    clearValue.color.float32[2] = clearColor[2];
    clearValue.color.float32[3] = clearColor[3];
}

void RenderPass::Descriptor::AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, std::int32_t clearColor[4])
{
    VkAttachmentDescription& desc = attachmentDescriptions_.EmplaceBack();

    desc.flags          = VK_FLAGS_NONE;
    desc.format         = Format2VK(imageFormat);
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = initialLayout;
    desc.finalLayout    = finalLayout;

    VkClearValue& clearValue = attachmentClears_.EmplaceBack();
    clearValue.color.int32[0] = clearColor[0];
    clearValue.color.int32[1] = clearColor[1];
    clearValue.color.int32[2] = clearColor[2];
    clearValue.color.int32[3] = clearColor[3];
}

void RenderPass::Descriptor::AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, float depthValue, std::uint32_t stencilValue)
{
    VkAttachmentDescription& desc = attachmentDescriptions_.EmplaceBack();

    desc.flags          = VK_FLAGS_NONE;
    desc.format         = Format2VK(imageFormat);
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = initialLayout;
    desc.finalLayout    = finalLayout;

    VkClearValue& clearValue = attachmentClears_.EmplaceBack();
    clearValue.depthStencil.depth = depthValue;
    clearValue.depthStencil.stencil = stencilValue;
}

void RenderPass::Descriptor::AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat, std::uint32_t clearColor[4])
{
    VkAttachmentDescription& desc = attachmentDescriptions_.EmplaceBack();

    desc.flags          = VK_FLAGS_NONE;
    desc.format         = Format2VK(imageFormat);
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = initialLayout;
    desc.finalLayout    = finalLayout;

    VkClearValue& clearValue = attachmentClears_.EmplaceBack();
    clearValue.color.uint32[0] = clearColor[0];
    clearValue.color.uint32[1] = clearColor[1];
    clearValue.color.uint32[2] = clearColor[2];
    clearValue.color.uint32[3] = clearColor[3];
}

void RenderPass::Descriptor::AddAttachment(VkImageLayout initialLayout, VkImageLayout finalLayout, Format imageFormat)
{
    VkAttachmentDescription& desc = attachmentDescriptions_.EmplaceBack();

    desc.flags          = VK_FLAGS_NONE;
    desc.format         = Format2VK(imageFormat);
    desc.samples        = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout  = initialLayout;
    desc.finalLayout    = finalLayout;

    VkClearValue& clearValue = attachmentClears_.EmplaceBack();
}

void RenderPass::Descriptor::BeginGraphicsSubpass()
{
    VkSubpassDescription& desc = subpassInfos_.EmplaceBack();
    desc.flags                      = VK_FLAGS_NONE;
    desc.pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS;
    desc.inputAttachmentCount       = 0;
    desc.colorAttachmentCount       = 0;
    desc.pColorAttachments          = nullptr;
    desc.pResolveAttachments        = nullptr;
    desc.pDepthStencilAttachment    = nullptr;
    desc.preserveAttachmentCount    = 0;
    desc.pPreserveAttachments       = nullptr;

    inputReferenceBeginPointer_ = inputReferences_.Size();
    inputReferencePerSubpassCount_ = 0;

    colorReferenceBeginPointer_ = colorReferences_.Size();
    colorReferencePerSubpassCount_ = 0;
}

void RenderPass::Descriptor::AddSubpassInput(std::uint8_t attachment, VkImageLayout layout)
{
    VkAttachmentReference& ref = inputReferences_.EmplaceBack();
    ref.attachment  = static_cast<std::uint32_t>(attachment);
    ref.layout      = layout;
    inputReferencePerSubpassCount_++;
}

void RenderPass::Descriptor::AddSubpassOutput(std::uint8_t attachment, VkImageLayout layout)
{
    VkAttachmentReference& ref = colorReferences_.EmplaceBack();
    ref.attachment  = static_cast<std::uint32_t>(attachment);
    ref.layout      = layout;
    colorReferencePerSubpassCount_++;
}

void RenderPass::Descriptor::AddSubpassDepthStencil(std::uint8_t attachment, VkImageLayout layout)
{
    VkSubpassDescription& desc = subpassInfos_[subpassInfos_.Size() - 1];

    DRE_ASSERT(desc.pDepthStencilAttachment == nullptr, "Subpass already has depthStencil reference.");

    depthStencilReference_.attachment   = attachment;
    depthStencilReference_.layout       = layout;

    desc.pDepthStencilAttachment = &depthStencilReference_;
}

void RenderPass::Descriptor::EndSubpass()
{
    VkSubpassDescription& desc = subpassInfos_[subpassInfos_.Size() - 1];

    desc.inputAttachmentCount = inputReferencePerSubpassCount_;
    desc.pInputAttachments = inputReferences_.Data() + inputReferenceBeginPointer_;

    desc.colorAttachmentCount = colorReferencePerSubpassCount_;
    desc.pColorAttachments = colorReferences_.Data() + colorReferenceBeginPointer_;
}

VkRenderPassCreateInfo const& RenderPass::Descriptor::CompileRenderPassCreateInfo()
{
    subpassDependencies_.ResizeUnsafe(subpassInfos_.Size() + 1);

    VkSubpassDependency& enterDependency = subpassDependencies_[0];
    enterDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    enterDependency.dstSubpass = 0;
    enterDependency.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    enterDependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    enterDependency.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    enterDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    enterDependency.dependencyFlags = VK_FLAGS_NONE;

    VkSubpassDependency& exitDependency = subpassDependencies_[subpassDependencies_.Size() - 1];
    exitDependency.srcSubpass = subpassInfos_.Size() - 1;
    exitDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    exitDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    exitDependency.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    exitDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    exitDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    exitDependency.dependencyFlags = VK_FLAGS_NONE;

    for (std::uint8_t i = 1; i < subpassInfos_.Size(); i++)
    {
        VkSubpassDependency& dependency = subpassDependencies_[i];
        dependency.srcSubpass = i - 1;
        dependency.dstSubpass = i;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dependencyFlags = VK_FLAGS_NONE;
    }

    createInfo_.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo_.pNext           = nullptr;
    createInfo_.flags           = VK_FLAGS_NONE;
    createInfo_.attachmentCount = attachmentDescriptions_.Size();
    createInfo_.pAttachments    = attachmentDescriptions_.Data();
    createInfo_.subpassCount    = subpassInfos_.Size();
    createInfo_.pSubpasses      = subpassInfos_.Data();
    createInfo_.dependencyCount = subpassDependencies_.Size();
    createInfo_.pDependencies   = subpassDependencies_.Data();

    return createInfo_;
}

RenderPass::RenderPass()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , attachmentCount_{ 0 }
    , subpassCount_{ 0 }
{
}

RenderPass::RenderPass(ImportTable* table, LogicalDevice* device, Descriptor& descriptor)
    : table_{ table }
    , device_{ device }
    , handle_{ VK_NULL_HANDLE }
    , attachmentCount_{ 0 }
    , subpassCount_{ 0 }
{
    VK_ASSERT(table_->vkCreateRenderPass(device_->Handle(), &descriptor.CompileRenderPassCreateInfo(), nullptr, &handle_));

    attachmentCount_ = descriptor.attachmentDescriptions_.Size();
    subpassCount_ = descriptor.subpassInfos_.Size();

    clearValues_ = descriptor.attachmentClears_;
}

RenderPass::RenderPass(RenderPass&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , attachmentCount_{ 0 }
    , subpassCount_{ 0 }
{
    operator=(std::move(rhs));
}

RenderPass& RenderPass::operator=(RenderPass&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(attachmentCount_);
    DRE_SWAP_MEMBER(subpassCount_);

    return *this;
}

RenderPass::~RenderPass()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyRenderPass(device_->Handle(), handle_, nullptr);
    }
}

void RenderPass::Begin(CommandList* commandList, Framebuffer* framebuffer)
{
    VkRenderPassBeginInfo info;
    info.sType          = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext          = nullptr;
    info.renderPass     = handle_;
    info.framebuffer    = framebuffer->GetHandle();
    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent.width = framebuffer->GetWidth();
    info.renderArea.extent.height = framebuffer->GetHeight();
    info.clearValueCount = attachmentCount_;
    info.pClearValues    = clearValues_.Data();

    table_->vkCmdBeginRenderPass(*commandList, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::NextSubpass(CommandList* commandList)
{
    table_->vkCmdNextSubpass(*commandList, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::End(CommandList* commandList)
{
    table_->vkCmdEndRenderPass(*commandList);
}


}