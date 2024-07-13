#include <vk_wrapper\Context.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\resources\Resource.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\RenderPass.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

namespace VKW
{

std::uint32_t constexpr BARRIER_MEM_SIZE = 256 * 1024;

Context::Context(VKW::ImportTable* table, VKW::Queue* queue, DRE::AllocatorLinear* barrierAllocator)
    : m_ImportTable{ table }
    , m_ParentQueue{ queue }
    , m_RenderingRect{}
    , m_PendingDependency{ barrierAllocator }
{
    m_CurrentCommandList = m_ParentQueue->GetFreeCommandList();
}

Context::~Context()
{
    m_ParentQueue->ReturnCommandList(m_CurrentCommandList);
}

void Context::ResetDependenciesVectors(DRE::AllocatorLinear* allocator)
{
    m_PendingDependency.Reset(allocator);
}

void Context::FlushAll()
{
    WriteResourceDependencies();
    m_ParentQueue->Execute(m_CurrentCommandList);
    m_CurrentCommandList = m_ParentQueue->GetFreeCommandList();
}

void Context::FlushOnlyPending()
{
    m_ParentQueue->ExecutePending();
}

void Context::WriteResourceDependencies()
{
    if (m_PendingDependency.IsEmpty())
        return;

    VkDependencyInfoKHR info;
    m_PendingDependency.GetDependency(info);
    m_ImportTable->vkCmdPipelineBarrier2(*m_CurrentCommandList, &info);
    m_PendingDependency.Clear();
}

void Context::FlushWaitSwapchain(PresentationContext& presentContext)
{
    WriteResourceDependencies();
    m_ParentQueue->ExecuteWaitSwapchain(m_CurrentCommandList, presentContext);
    m_CurrentCommandList = m_ParentQueue->GetFreeCommandList();
}

void Context::Present(PresentationContext& presentContext)
{
    presentContext.Present(m_ParentQueue);
}

VKW::QueueExecutionPoint Context::SyncPoint(std::uint8_t waitCount, VKW::QueueExecutionPoint const* waits)
{
    DRE_ASSERT(m_CurrentCommandList != nullptr, "Failed to submit CommandLists, current CmdList is nullptr.");

    WriteResourceDependencies();
    VKW::QueueExecutionPoint point = m_ParentQueue->ScheduleExecute(m_CurrentCommandList, waitCount, waits);
    m_ParentQueue->ExecutePending();
    m_CurrentCommandList = m_ParentQueue->GetFreeCommandList();

    return point;
}

VKW::QueueExecutionPoint Context::SyncPoint(VKW::QueueExecutionPoint const& wait)
{
    return SyncPoint(1, &wait);
}

void Context::CmdDraw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance)
{
    m_ImportTable->vkCmdDraw(*m_CurrentCommandList, vertexCount, instanceCount, firstVertex, firstInstance);
}

void Context::CmdDrawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance)
{
    m_ImportTable->vkCmdDrawIndexed(*m_CurrentCommandList, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void Context::CmdDispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    WriteResourceDependencies();
    m_ImportTable->vkCmdDispatch(*m_CurrentCommandList, x, y, z);
}

void Context::CmdBindVertexBuffer(VKW::BufferResource const* buffer, std::uint32_t offset)
{
    VkDeviceSize const offsetDevice = static_cast<VkDeviceSize>(offset);
    VkBuffer handle = buffer->handle_;
    m_ImportTable->vkCmdBindVertexBuffers(*m_CurrentCommandList, 0, 1, &handle, &offsetDevice);
}

void Context::CmdBindIndexBuffer(VKW::BufferResource const* buffer, std::uint32_t offset)
{
    VkDeviceSize const offsetDevice = static_cast<VkDeviceSize>(offset);
    VkBuffer handle = buffer->handle_;
    m_ImportTable->vkCmdBindIndexBuffer(*m_CurrentCommandList, handle, offsetDevice, VK_INDEX_TYPE_UINT32);
}

void Context::CmdBindPipeline(BindPoint bindPoint, VKW::Pipeline const* pipeline)
{
    VkPipelineBindPoint const vkBindPoint = (bindPoint == BindPoint::Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
    m_ImportTable->vkCmdBindPipeline(*m_CurrentCommandList, vkBindPoint, pipeline->GetHandle());
}

void Context::CmdBindGraphicsPipeline(VKW::Pipeline const* pipeline)
{
    CmdBindPipeline(BindPoint::Graphics, pipeline);
}

void Context::CmdBindComputePipeline(VKW::Pipeline const* pipeline)
{
    CmdBindPipeline(BindPoint::Compute, pipeline);
}

void Context::CmdBindDescriptorSets(
    VKW::PipelineLayout const* layout, BindPoint bindPoint,
    std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
    std::uint32_t dynamicOffsetCount, std::uint32_t const* pDynamicOffsets)
{
    VkPipelineBindPoint const vkBindPoint = (bindPoint == BindPoint::Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

    VkDescriptorSet vkSets[VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS];
    for (std::uint32_t i = 0; i < descriptorSetCount; i++)
    {
        vkSets[i] = sets->GetHandle();
    }

    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, vkBindPoint, layout->GetHandle(), firstSet, descriptorSetCount, vkSets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindGraphicsDescriptorSets(
    VKW::PipelineLayout const* layout,
    std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
    std::uint32_t dynamicOffsetCount, std::uint32_t const* pDynamicOffsets)
{
    CmdBindDescriptorSets(layout, BindPoint::Graphics, firstSet, descriptorSetCount, sets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindComputeDescriptorSets(
    VKW::PipelineLayout const* layout,
    std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
    std::uint32_t dynamicOffsetCount, std::uint32_t const* pDynamicOffsets)
{
    CmdBindDescriptorSets(layout, BindPoint::Compute, firstSet, descriptorSetCount, sets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindGlobalDescriptorSets(VKW::DescriptorManager& descriptorManager, std::uint8_t frameID)
{
    VkDescriptorSet globalSets[3];
    globalSets[0] = descriptorManager.GetGlobalSampler().GetHandle();
    globalSets[1] = descriptorManager.GetGlobalTexturesSet().GetHandle();
    globalSets[2] = descriptorManager.GetGlobalUniformSet(frameID).GetHandle();

    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorManager.GetGlobalPipelineLayout()->GetHandle(), 0, 3, globalSets, 0, nullptr);
    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, VK_PIPELINE_BIND_POINT_COMPUTE, descriptorManager.GetGlobalPipelineLayout()->GetHandle(), 0, 3, globalSets, 0, nullptr);
}

void Context::CmdSetViewport(std::uint32_t viewportCount, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    VkViewport vp[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
    for (std::uint32_t i = 0; i < viewportCount; i++)
    {
        vp[i].x         = static_cast<float>(x);
        vp[i].y         = static_cast<float>(y);
        vp[i].width     = static_cast<float>(width);
        vp[i].height    = static_cast<float>(height);
        vp[i].minDepth  = static_cast<float>(0.0f);
        vp[i].maxDepth  = static_cast<float>(1.0f);
    }

    m_ImportTable->vkCmdSetViewport(*m_CurrentCommandList, 0, viewportCount, vp);
}

void Context::CmdSetScissor(std::uint32_t scissorCount, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    VkRect2D sc[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
    for (std::uint32_t i = 0; i < scissorCount; i++)
    {
        sc[i].offset.x = x;
        sc[i].offset.y = y;
        sc[i].extent.width = width;
        sc[i].extent.height = height;
    }

    m_ImportTable->vkCmdSetScissor(*m_CurrentCommandList, 0, scissorCount, sc);
}

#ifndef DRE_COMPILE_FOR_RENDERDOC
void Context::CmdSetPolygonMode(PolygonModeBits mode)
{
    VkPolygonMode const vkMode = mode == POLYGON_WIREFRAME ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    m_ImportTable->vkCmdSetPolygonModeEXT(*m_CurrentCommandList, vkMode);
}
#endif // DRE_COMPILE_FOR_RENDERDOC

void Context::CmdPushConstants(VKW::PipelineLayout const* layout, VKW::DescriptorStage stages, std::uint32_t offset, std::uint32_t size, void const* pValues)
{
    VkShaderStageFlags const shaderStages = VKW::HELPER::DescriptorStageToVK(stages);
    m_ImportTable->vkCmdPushConstants(*m_CurrentCommandList, layout->GetHandle(), shaderStages, offset, size, pValues);
}

void Context::CmdResourceDependency(VKW::ImageResource const* resource,
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    std::uint32_t const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdResourceDependency(VKW::BufferResource const* resource,
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    std::uint32_t const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdResourceDependency(VKW::BufferResource const* resource,
    std::uint32_t offset, std::uint32_t size, // offset is added to the base offset of the VKW::BufferResource
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    std::uint32_t const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        offset, size,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdClearAttachments(AttachmentMask attachments, float color[4])
{
    VkClearValue value{};
    value.color.float32[0] = color[0];
    value.color.float32[1] = color[1];
    value.color.float32[2] = color[2];
    value.color.float32[3] = color[3];

    DRE::InplaceVector<VkClearAttachment, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> clears;
    DRE::InplaceVector<VkClearRect, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> rects;
    for (std::uint32_t i = 0; i < VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS; i++)
    {
        if (attachments & (ATTACHMENT_MASK_COLOR_0 << i))
        {
            VkClearAttachment& clear = clears.EmplaceBack();
            clear.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
            clear.colorAttachment   = i;
            clear.clearValue        = value;

            rects.EmplaceBack(m_RenderingRect, 0u, 1u);
        }
    }

    m_ImportTable->vkCmdClearAttachments(*m_CurrentCommandList, clears.Size(), clears.Data(), rects.Size(), rects.Data());
}

void Context::CmdClearAttachments(AttachmentMask attachments, float depth, std::uint32_t stencil)
{
    VkClearValue value{};
    value.depthStencil.depth = depth;
    value.depthStencil.stencil = stencil;

    DRE::InplaceVector<VkClearAttachment, 2> clears;
    DRE::InplaceVector<VkClearRect, 2> rects;
    
    if (attachments & ATTACHMENT_MASK_DEPTH)
    {
        VkClearAttachment& clear = clears.EmplaceBack();
        clear.aspectMask        = VK_IMAGE_ASPECT_DEPTH_BIT;
        clear.colorAttachment   = 0;
        clear.clearValue        = value;
        rects.EmplaceBack(m_RenderingRect, 0u, 1u);
    }

    if (attachments & ATTACHMENT_MASK_STENCIL)
    {
        VkClearAttachment& clear = clears.EmplaceBack();
        clear.aspectMask        = VK_IMAGE_ASPECT_STENCIL_BIT;
        clear.colorAttachment   = 0;
        clear.clearValue        = value;
        rects.EmplaceBack(m_RenderingRect, 0u, 1u);
    }

    m_ImportTable->vkCmdClearAttachments(*m_CurrentCommandList, clears.Size(), clears.Data(), rects.Size(), rects.Data());
}

void Context::CmdBeginRendering(std::uint32_t attachmentCount, VKW::ImageResourceView* const* attachments,
    VKW::ImageResourceView const* depthAttachment, VKW::ImageResourceView const* stencilAttachment)
{
    DRE_ASSERT(attachmentCount <= VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS, "Exceeded maximum color attachment count.");
    
    std::uint32_t renderingWidth = 0;
    std::uint32_t renderingHeight = 0;

    if (attachmentCount > 0)
    {
        renderingWidth = attachments[0]->GetImageWidth();
        renderingHeight = attachments[0]->GetImageHeight();
    }

    if (renderingWidth == 0 && depthAttachment != nullptr)
    {
        renderingWidth = depthAttachment->GetImageWidth();
        renderingHeight = depthAttachment->GetImageHeight();
    }

    if (renderingWidth == 0 && stencilAttachment != nullptr)
    {
        renderingWidth = stencilAttachment->GetImageWidth();
        renderingHeight = stencilAttachment->GetImageHeight();
    }

    DRE_ASSERT(renderingWidth > 0 && renderingHeight > 0, "Invalid Rendering extent or no rendering attachments were provided!");

    m_RenderingRect.offset.x = 0;
    m_RenderingRect.offset.y = 0;
    m_RenderingRect.extent.width = renderingWidth;
    m_RenderingRect.extent.height = renderingHeight;

    DRE::InplaceVector<VkRenderingAttachmentInfoKHR, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> colorInfos;
    for (std::uint32_t i = 0; i < attachmentCount; i++)
    {
        VkRenderingAttachmentInfoKHR& attachmentInfo = colorInfos.EmplaceBack();
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        attachmentInfo.pNext = nullptr;
        attachmentInfo.imageView = attachments[i]->handle_;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        attachmentInfo.resolveImageView = VK_NULL_HANDLE;
        attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentInfo.clearValue = {};
    }

    VkRenderingAttachmentInfoKHR depthInfo;
    if (depthAttachment != nullptr)
    {
        depthInfo.sType                 = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthInfo.pNext                 = nullptr;
        depthInfo.imageView             = depthAttachment->handle_;
        depthInfo.imageLayout           = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        depthInfo.resolveMode           = VK_RESOLVE_MODE_NONE;
        depthInfo.resolveImageView      = VK_NULL_HANDLE;
        depthInfo.resolveImageLayout    = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        depthInfo.loadOp                = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthInfo.storeOp               = VK_ATTACHMENT_STORE_OP_STORE;
        depthInfo.clearValue            = {};
    }

    VkRenderingAttachmentInfoKHR stencilInfo;
    if (stencilAttachment != nullptr)
    {
        stencilInfo.sType               = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        stencilInfo.pNext               = nullptr;
        stencilInfo.imageView           = stencilAttachment->handle_;
        stencilInfo.imageLayout         = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        stencilInfo.resolveMode         = VK_RESOLVE_MODE_NONE;
        stencilInfo.resolveImageView    = VK_NULL_HANDLE;
        stencilInfo.resolveImageLayout  = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        stencilInfo.loadOp              = VK_ATTACHMENT_LOAD_OP_LOAD;
        stencilInfo.storeOp             = VK_ATTACHMENT_STORE_OP_STORE;
        stencilInfo.clearValue          = {};
    }

    VkRenderingInfoKHR info;
    info.sType                          = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    info.pNext                          = nullptr;
    info.flags                          = VK_FLAGS_NONE;
    info.renderArea.offset.x            = 0;
    info.renderArea.offset.y            = 0;
    info.renderArea.extent.width        = renderingWidth;
    info.renderArea.extent.height       = renderingHeight;
    info.layerCount                     = 1;
    info.viewMask                       = 0;
    info.colorAttachmentCount           = colorInfos.Size();
    info.pColorAttachments              = colorInfos.Data();
    info.pDepthAttachment               = depthAttachment ? &depthInfo : nullptr;
    info.pStencilAttachment             = stencilAttachment ? &stencilInfo : nullptr;

    WriteResourceDependencies();
    m_ImportTable->vkCmdBeginRendering(*m_CurrentCommandList, &info);
}

void Context::CmdEndRendering()
{
    m_RenderingRect.offset.x = 0;
    m_RenderingRect.offset.y = 0;
    m_RenderingRect.extent.width = 0;
    m_RenderingRect.extent.height = 0;

    m_ImportTable->vkCmdEndRendering(*m_CurrentCommandList);
}

void Context::CmdBindVertexBuffer(VKW::BufferResource* vertexBuffer, std::uint32_t offset)
{
    VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);
    m_ImportTable->vkCmdBindVertexBuffers(*m_CurrentCommandList, 0, 1, &vertexBuffer->handle_, &vkOffset);
}

void Context::CmdBindIndexBuffer(VKW::BufferResource* indexBuffer, std::uint32_t offset, std::uint8_t indexSize)
{
    m_ImportTable->vkCmdBindIndexBuffer(*m_CurrentCommandList, indexBuffer->handle_, static_cast<VkDeviceSize>(offset), indexSize == 16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void Context::CmdClearColorImage(VKW::ImageResource const* image, float color[4])
{
    VkImageSubresourceRange const range = VKW::HELPER::DefaultImageSubresourceRange();
    VkClearColorValue value{};
    value.float32[0] = color[0];
    value.float32[1] = color[1];
    value.float32[2] = color[2];
    value.float32[3] = color[3];

    WriteResourceDependencies();
    m_ImportTable->vkCmdClearColorImage(*m_CurrentCommandList, image->handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
}

void Context::CmdClearDepthStencilImage(VKW::ImageResource const* image, float depth, std::uint32_t stencil)
{
    VkImageSubresourceRange const range = VKW::HELPER::DefaultImageSubresourceRange();
    VkClearDepthStencilValue value{};
    value.depth = depth;
    value.stencil = stencil;

    WriteResourceDependencies();
    m_ImportTable->vkCmdClearDepthStencilImage(*m_CurrentCommandList, image->handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
}

void Context::CmdCopyImageToImage(VKW::ImageResource const* dst, VKW::ImageResource const* src)
{
    VkImageCopy2KHR region{};
    VkCopyImageInfo2KHR info{};
    info.sType          = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2_KHR;
    info.pNext          = nullptr;
    info.srcImage       = src->handle_;
    info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    info.dstImage       = dst->handle_;
    info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    info.regionCount    = 1;
    info.pRegions       = &region;

    region.sType            = VK_STRUCTURE_TYPE_IMAGE_COPY_2_KHR;
    region.pNext            = nullptr;
    region.srcSubresource   = HELPER::DefaultImageSubresourceLayers();
    region.srcOffset        = VkOffset3D{ 0, 0, 0 };
    region.dstSubresource   = HELPER::DefaultImageSubresourceLayers();
    region.dstOffset        = VkOffset3D{ 0, 0, 0 };
    region.extent           = VkExtent3D{ dst->width_, dst->height_, 1 };

    WriteResourceDependencies();
    m_ImportTable->vkCmdCopyImage2(*m_CurrentCommandList, &info);
}

void Context::CmdCopyBufferToImage(VKW::ImageResource const* dst, VKW::BufferResource const* src, std::uint32_t bufferOffset)
{
    VkBufferImageCopy2KHR copyDesc{};
    copyDesc.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2_KHR;
    copyDesc.pNext = nullptr;
    copyDesc.bufferOffset = bufferOffset;
    copyDesc.bufferRowLength = dst->width_;
    copyDesc.bufferImageHeight = dst->height_;
    copyDesc.imageSubresource = VKW::HELPER::DefaultImageSubresourceLayers();
    copyDesc.imageOffset.x = 0;
    copyDesc.imageOffset.y = 0;
    copyDesc.imageOffset.z = 0;
    copyDesc.imageExtent.width = dst->width_;
    copyDesc.imageExtent.height = dst->height_;
    copyDesc.imageExtent.depth = 1;

    VkCopyBufferToImageInfo2KHR copyInfo;
    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2_KHR;
    copyInfo.pNext = nullptr;
    copyInfo.srcBuffer = src->handle_;
    copyInfo.dstImage = dst->handle_;
    copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &copyDesc;

    WriteResourceDependencies();
    m_ImportTable->vkCmdCopyBufferToImage2(*m_CurrentCommandList, &copyInfo);
}

void Context::CmdCopyBufferToBuffer(VKW::BufferResource const* dst, std::uint32_t dstOffset, VKW::BufferResource const* src, std::uint32_t srcOffset, std::uint32_t size)
{
    VkBufferCopy2 region;
    region.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
    region.pNext = nullptr;
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    region.size = size;

    VkCopyBufferInfo2 info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
    info.pNext = nullptr;
    info.srcBuffer = src->handle_;
    info.dstBuffer = dst->handle_;
    info.regionCount = 1;
    info.pRegions = &region;

    WriteResourceDependencies();
    m_ImportTable->vkCmdCopyBuffer2(*m_CurrentCommandList, &info);
}

void Context::WaitIdle()
{
    m_ParentQueue->WaitIdle();
}

}


