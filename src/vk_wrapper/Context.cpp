#include <vk_wrapper\Context.hpp>

#include <foundation\memory\Memory.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\Resource.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\RenderPass.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

namespace VKW
{

DRE::U32 constexpr BARRIER_MEM_SIZE = 256 * 1024;

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
    FlushOnlyPending();
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
    FlushOnlyPending();
    m_ParentQueue->ExecuteWaitSwapchain(m_CurrentCommandList, presentContext);
    m_CurrentCommandList = m_ParentQueue->GetFreeCommandList();
}

void Context::Present(PresentationContext& presentContext)
{
    presentContext.Present(m_ParentQueue);
}

VKW::QueueExecutionPoint Context::SyncPoint(DRE::U8 waitCount, VKW::QueueExecutionPoint const* waits)
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

void Context::CmdDraw(DRE::U32 vertexCount, DRE::U32 instanceCount, DRE::U32 firstVertex, DRE::U32 firstInstance)
{
    m_ImportTable->vkCmdDraw(*m_CurrentCommandList, vertexCount, instanceCount, firstVertex, firstInstance);
}

void Context::CmdDrawIndirect(VKW::BufferResource const* buffer, DRE::U32 offset, DRE::U32 drawCount, DRE::U32 stride)
{
    m_ImportTable->vkCmdDrawIndirect(*m_CurrentCommandList, buffer->handle_, offset, drawCount, stride);
}

void Context::CmdDrawIndexed(DRE::U32 indexCount, DRE::U32 instanceCount, DRE::U32 firstIndex, DRE::S32 vertexOffset, DRE::U32 firstInstance)
{
    m_ImportTable->vkCmdDrawIndexed(*m_CurrentCommandList, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void Context::CmdDrawIndexedIndirect(VKW::BufferResource const* buffer, DRE::U32 offset, DRE::U32 drawCount, DRE::U32 stride)
{
    m_ImportTable->vkCmdDrawIndexedIndirect(*m_CurrentCommandList, buffer->handle_, offset, drawCount, stride);
}

void Context::CmdDispatch(DRE::U32 x, DRE::U32 y, DRE::U32 z)
{
    WriteResourceDependencies();
    m_ImportTable->vkCmdDispatch(*m_CurrentCommandList, x, y, z);
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
    DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
    DRE::U32 dynamicOffsetCount, DRE::U32 const* pDynamicOffsets)
{
    VkPipelineBindPoint const vkBindPoint = (bindPoint == BindPoint::Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

    VkDescriptorSet vkSets[VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS];
    for (DRE::U32 i = 0; i < descriptorSetCount; i++)
    {
        vkSets[i] = sets->GetHandle();
    }

    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, vkBindPoint, layout->GetHandle(), firstSet, descriptorSetCount, vkSets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindGraphicsDescriptorSets(
    VKW::PipelineLayout const* layout,
    DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
    DRE::U32 dynamicOffsetCount, DRE::U32 const* pDynamicOffsets)
{
    CmdBindDescriptorSets(layout, BindPoint::Graphics, firstSet, descriptorSetCount, sets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindComputeDescriptorSets(
    VKW::PipelineLayout const* layout,
    DRE::U32 firstSet, DRE::U32 descriptorSetCount, VKW::DescriptorSet const* sets,
    DRE::U32 dynamicOffsetCount, DRE::U32 const* pDynamicOffsets)
{
    CmdBindDescriptorSets(layout, BindPoint::Compute, firstSet, descriptorSetCount, sets, dynamicOffsetCount, pDynamicOffsets);
}

void Context::CmdBindGlobalDescriptorSets(VKW::DescriptorManager& descriptorManager, DRE::U8 frameID)
{
    VkDescriptorSet globalSets[3];
    globalSets[0] = descriptorManager.GetGlobalGenericSet().GetHandle();
    globalSets[1] = descriptorManager.GetGlobalTexturesSet().GetHandle();
    globalSets[2] = descriptorManager.GetGlobalUniformSet(frameID).GetHandle();

    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorManager.GetGlobalPipelineLayout()->GetHandle(), 0, 3, globalSets, 0, nullptr);
    m_ImportTable->vkCmdBindDescriptorSets(*m_CurrentCommandList, VK_PIPELINE_BIND_POINT_COMPUTE, descriptorManager.GetGlobalPipelineLayout()->GetHandle(), 0, 3, globalSets, 0, nullptr);
}

void Context::CmdSetViewport(DRE::U32 viewportCount, DRE::U32 x, DRE::U32 y, DRE::U32 width, DRE::U32 height)
{
    VkViewport vp[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
    for (DRE::U32 i = 0; i < viewportCount; i++)
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

void Context::CmdSetScissor(DRE::U32 scissorCount, DRE::U32 x, DRE::U32 y, DRE::U32 width, DRE::U32 height)
{
    VkRect2D sc[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
    for (DRE::U32 i = 0; i < scissorCount; i++)
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

void Context::CmdPushConstants(VKW::PipelineLayout const* layout, VKW::DescriptorStage stages, DRE::U32 offset, DRE::U32 size, void const* pValues)
{
    VkShaderStageFlags const shaderStages = VKW::HELPER::DescriptorStageToVK(stages);
    m_ImportTable->vkCmdPushConstants(*m_CurrentCommandList, layout->GetHandle(), shaderStages, offset, size, pValues);
}

void Context::CmdMemoryDependency(
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    m_PendingDependency.Add(
        srcAccess, srcStage,
        dstAccess, dstStage
    );
}

void Context::CmdResourceDependency(VKW::ImageResource const* resource,
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    DRE::U32 const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdResourceDependency(VKW::BufferResource const* resource,
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    DRE::U32 const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdResourceDependency(VKW::BufferResource const* resource,
    DRE::U32 offset, DRE::U32 size, // offset is added to the base offset of the VKW::BufferResource
    ResourceAccess srcAccess, Stages srcStage,
    ResourceAccess dstAccess, Stages dstStage)
{
    DRE::U32 const queueFamily = m_ParentQueue->GetQueueFamily();

    m_PendingDependency.Add(resource,
        offset, size,
        srcAccess, srcStage, queueFamily,
        dstAccess, dstStage, queueFamily);
}

void Context::CmdClearAttachments(AttachmentMask attachments, DRE::U32* value)
{
    VkClearValue clearValue{};
    clearValue.color.uint32[0] = value[0];
    clearValue.color.uint32[1] = value[1];
    clearValue.color.uint32[2] = value[2];
    clearValue.color.uint32[3] = value[3];

    DRE::InplaceVector<VkClearAttachment, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> clears;
    DRE::InplaceVector<VkClearRect, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> rects;
    for (DRE::U32 i = 0; i < VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS; i++)
    {
        if (attachments & (ATTACHMENT_MASK_COLOR_0 << i))
        {
            VkClearAttachment& clear = clears.EmplaceBack();
            clear.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
            clear.colorAttachment   = i;
            clear.clearValue        = clearValue;

            rects.EmplaceBack(m_RenderingRect, 0u, 1u);
        }
    }

    m_ImportTable->vkCmdClearAttachments(*m_CurrentCommandList, clears.Size(), clears.Data(), rects.Size(), rects.Data());
}

void Context::CmdClearAttachments(AttachmentMask attachments, float* color)
{
    VkClearValue value{};
    value.color.float32[0] = color[0];
    value.color.float32[1] = color[1];
    value.color.float32[2] = color[2];
    value.color.float32[3] = color[3];

    DRE::InplaceVector<VkClearAttachment, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> clears;
    DRE::InplaceVector<VkClearRect, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> rects;
    for (DRE::U32 i = 0; i < VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS; i++)
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

void Context::CmdClearAttachments(AttachmentMask attachments, float depth, DRE::U32 stencil)
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

void Context::CmdBeginRendering(DRE::U32 attachmentCount, VKW::ImageResourceView* const* attachments,
    VKW::ImageResourceView const* depthAttachment, VKW::ImageResourceView const* stencilAttachment)
{
    DRE_ASSERT(attachmentCount <= VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS, "Exceeded maximum color attachment count.");

    DRE::U32 renderingWidth = 0;
    DRE::U32 renderingHeight = 0;

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
    for (DRE::U32 i = 0; i < attachmentCount; i++)
    {
        VkRenderingAttachmentInfoKHR& attachmentInfo = colorInfos.EmplaceBack();
        attachmentInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        attachmentInfo.pNext                = nullptr;
        attachmentInfo.imageView            = attachments[i]->handle_;
        attachmentInfo.imageLayout          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.resolveMode          = VK_RESOLVE_MODE_NONE;
        attachmentInfo.resolveImageView     = VK_NULL_HANDLE;
        attachmentInfo.resolveImageLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        attachmentInfo.loadOp               = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentInfo.storeOp              = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentInfo.clearValue           = {};
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

void Context::CmdBindVertexBuffer(VKW::BufferResource const* vertexBuffer, DRE::U32 offset)
{
    VkDeviceSize vkOffset = static_cast<VkDeviceSize>(offset);
    m_ImportTable->vkCmdBindVertexBuffers(*m_CurrentCommandList, 0, 1, &vertexBuffer->handle_, &vkOffset);
}

void Context::CmdBindIndexBuffer(VKW::BufferResource const* indexBuffer, DRE::U32 offset, DRE::U8 indexSize)
{
    m_ImportTable->vkCmdBindIndexBuffer(*m_CurrentCommandList, indexBuffer->handle_, static_cast<VkDeviceSize>(offset), indexSize == 16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void Context::CmdClearColorImage(VKW::ImageResource const* image, float color[4])
{
    VkImageSubresourceRange const range = VKW::HELPER::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, image->mipLevels_);
    VkClearColorValue value{};
    value.float32[0] = color[0];
    value.float32[1] = color[1];
    value.float32[2] = color[2];
    value.float32[3] = color[3];

    WriteResourceDependencies();
    m_ImportTable->vkCmdClearColorImage(*m_CurrentCommandList, image->handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
}

void Context::CmdClearDepthStencilImage(VKW::ImageResource const* image, float depth, DRE::U32 stencil)
{
    VkImageSubresourceRange const range = VKW::HELPER::ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 1);
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

void Context::CmdCopyBufferToImage(VKW::ImageResource const* dst, VKW::BufferResource const* src, DRE::U32 bufferOffset)
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

void Context::CmdCopyImageToBuffer(VKW::BufferResource const* dst, VKW::ImageResource const* src, DRE::U32 bufferOffset)
{
    VkBufferImageCopy2KHR copyDesc{};
    copyDesc.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2_KHR;
    copyDesc.pNext = nullptr;
    copyDesc.bufferOffset = bufferOffset;
    copyDesc.bufferRowLength = src->width_;
    copyDesc.bufferImageHeight = src->height_;
    copyDesc.imageSubresource = VKW::HELPER::DefaultImageSubresourceLayers();
    copyDesc.imageOffset.x = 0;
    copyDesc.imageOffset.y = 0;
    copyDesc.imageOffset.z = 0;
    copyDesc.imageExtent.width = src->width_;
    copyDesc.imageExtent.height = src->height_;
    copyDesc.imageExtent.depth = 1;

    VkCopyImageToBufferInfo2KHR copyInfo;
    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2_KHR;
    copyInfo.pNext = nullptr;
    copyInfo.srcImage = src->handle_;
    copyInfo.dstBuffer = dst->handle_;
    copyInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &copyDesc;

    WriteResourceDependencies();
    m_ImportTable->vkCmdCopyImageToBuffer2(*m_CurrentCommandList, &copyInfo);
}

void Context::CmdCopyBufferToBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, VKW::BufferResource const* src, DRE::U32 srcOffset, DRE::U32 size)
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

void Context::CmdFillBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, DRE::U32 size, DRE::U32 data)
{
    WriteResourceDependencies();
    m_ImportTable->vkCmdFillBuffer(*m_CurrentCommandList, dst->handle_, dstOffset, size, data);
}

void Context::CmdUpdateBuffer(VKW::BufferResource const* dst, DRE::U32 dstOffset, DRE::U32 dataSize, void const* pData)
{
    DRE_ASSERT(dataSize < 65536, "Data size for CmdUpdateBuffer must be less than 65536 bytes according to the Vulkan specification.");

    WriteResourceDependencies();
    m_ImportTable->vkCmdUpdateBuffer(*m_CurrentCommandList, dst->handle_, dstOffset, dataSize, pData);
}

void Context::CmdBuildBLAS(VKW::AccelerationStructureResource const* blas,
    DRE::U64 scratchBufferAddress,
    DRE::U64 vertexBufferAddress,
    DRE::U64 vertexStride,
    DRE::U64 vertexCount,
    DRE::U64 indexBufferAddress,
    DRE::U64 indexCount)
{
    VkAccelerationStructureGeometryKHR geometryInfo;
    geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometryInfo.pNext = nullptr;
    geometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometryInfo.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometryInfo.geometry.triangles.pNext = nullptr;
    geometryInfo.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometryInfo.geometry.triangles.vertexData.deviceAddress = vertexBufferAddress;
    geometryInfo.geometry.triangles.vertexStride = vertexStride;
    geometryInfo.geometry.triangles.maxVertex = vertexCount - 1; // yep, -1 is according to the spec
    geometryInfo.geometry.triangles.indexData.deviceAddress = indexBufferAddress;
    geometryInfo.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometryInfo.geometry.triangles.transformData.deviceAddress = 0;
    geometryInfo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // WARNING, DIDN'T HAVE ANY HITS WITHOUT IT FOR SHADOWS

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.pNext = nullptr;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_FLAGS_NONE;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = blas->handle_;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometryInfo;
    buildInfo.ppGeometries = nullptr;
    buildInfo.scratchData.deviceAddress = scratchBufferAddress;

    VkAccelerationStructureBuildRangeInfoKHR buildRange;
    buildRange.primitiveCount = indexCount / 3;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* buildRangePtr = &buildRange;

    m_ImportTable->vkCmdBuildAccelerationStructuresKHR(*m_CurrentCommandList,
        1,
        &buildInfo,
        &buildRangePtr);
}

void Context::CmdBuildTLAS(
    VKW::AccelerationStructureResource const* tlas,
    DRE::U64 scratchBufferAddress,
    DRE::U32 instanceCount,
    DRE::U64 instanceBufferAddress)
{
    VkAccelerationStructureGeometryKHR geometry;
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.pNext = nullptr;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.pNext = nullptr;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data.deviceAddress = instanceBufferAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.pNext = nullptr;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = tlas->handle_;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    buildInfo.ppGeometries = nullptr;
    buildInfo.scratchData.deviceAddress = scratchBufferAddress;

    VkAccelerationStructureBuildRangeInfoKHR buildRange;
    buildRange.primitiveCount = instanceCount;
    buildRange.primitiveOffset = 0;
    buildRange.firstVertex = 0;
    buildRange.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* buildRangePtr = &buildRange;

    m_ImportTable->vkCmdBuildAccelerationStructuresKHR(*m_CurrentCommandList,
        1,
        &buildInfo,
        &buildRangePtr);
}

void Context::CmdBeginDebugLabel(char const* label)
{
#ifdef DRE_DEBUG
    VkDebugUtilsLabelEXT sLabel;
    sLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    sLabel.pNext = nullptr;
    sLabel.pLabelName = label;
    sLabel.color[0] = 1.0f;
    sLabel.color[1] = 1.0f;
    sLabel.color[2] = 1.0f;
    sLabel.color[3] = 1.0f;

    m_ImportTable->vkCmdBeginDebugUtilsLabelEXT(*m_CurrentCommandList, &sLabel);
#endif // DRE_DEBUG
}

void Context::CmdEndDebugLabel()
{
#ifdef DRE_DEBUG
    m_ImportTable->vkCmdEndDebugUtilsLabelEXT(*m_CurrentCommandList);
#endif // DRE_DEBUG
}

void Context::CmdInsertDebugLabel(char const* label)
{
#ifdef DRE_DEBUG
    VkDebugUtilsLabelEXT sLabel;
    sLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    sLabel.pNext = nullptr;
    sLabel.pLabelName = label;
    sLabel.color[0] = 1.0f;
    sLabel.color[1] = 1.0f;
    sLabel.color[2] = 1.0f;
    sLabel.color[3] = 1.0f;

    m_ImportTable->vkCmdInsertDebugUtilsLabelEXT(*m_CurrentCommandList, &sLabel);
#endif // DRE_DEBUG
}

void Context::WaitIdle()
{
    m_ParentQueue->WaitIdle();
}

bool Context::TestLayoutCompatibility(VKW::PipelineLayout const* parentLayout, VKW::PipelineLayout const* childLayout)
{
    bool compatible = true;

#ifdef DEBUG_LAYOUT_MEMBERS
    if (childLayout->GetMemberCount() < parentLayout->GetMemberCount())
    {
        std::cerr << "child layout must have at least the same number of members" << std::endl;
        return false;
    }

    if (parentLayout->GetDescriptor().GetPushConstantsCount() != childLayout->GetDescriptor().GetPushConstantsCount())
    {
        compatible = false;
        std::cerr << "Incompatible pipeline layout. parent pushConstantCount==" << (DRE::U32)parentLayout->GetDescriptor().GetPushConstantsCount()
        << " but child pushConstantCount==" << (DRE::U32)childLayout->GetDescriptor().GetPushConstantsCount() << std::endl;
    }
    else
    {
        for (DRE::U32 i = 0, size = parentLayout->GetDescriptor().GetPushConstantsCount(); i < size; i++)
        {
#define check_property(prop) \
            if (parentLayout->GetDescriptor().GetPushConstant(i).##prop != childLayout->GetDescriptor().GetPushConstant(i).##prop)\
            {\
                std::cerr << "Incompatible pipeline layout. parent pushConstant" << i << "."#prop"==" << parentLayout->GetDescriptor().GetPushConstant(i).##prop\
                    << " but child ." #prop"==" << childLayout->GetDescriptor().GetPushConstant(i).##prop << std::endl;\
                compatible = false;\
            }

            check_property(stageFlags);
            check_property(offset);
            check_property(size);

#undef check_property
        }

        if (!compatible)
            return false;
    }



    for (DRE::U32 i = 0, size = parentLayout->GetMemberCount(); i < size; i++)
    {
        auto* layout1 = parentLayout->GetMember(i);
        auto* layout2 = childLayout->GetMember(i);

        if (layout1->GetDescriptor().GetCount() != layout2->GetDescriptor().GetCount())
        {
            std::cerr << "Incompatible pipeline layout. DescriptorLayout " << i << " has different member count. Parent:"
                << layout1->GetDescriptor().GetCount() << ", Child:" << layout2->GetDescriptor().GetCount() << std::endl;
            compatible = false;
        }

        for (DRE::U32 ii = 0; ii < layout1->GetDescriptor().GetCount(); ii++)
        {
            auto& m1 = layout1->GetDescriptor().GetMember(ii);
            auto& m2 = layout2->GetDescriptor().GetMember(ii);

#define check_property(prop) if(m1.##prop != m2.##prop) {\
            std::cerr << "Incompatible pipeline layout. DescriptorLayout " << i << ", member " << ii\
            << ".\nparent."#prop"==" << m1.##prop << ", but child."#prop"==" << m2.##prop << std::endl;\
            compatible = false;\
            }

            check_property(type_);
            check_property(binding_);
            check_property(stage_);
            check_property(count_);
            check_property(variableCount_);
            check_property(updateAfterBind_);

        }
#undef check_property
        if (!compatible)
            return false;

#endif // DEBUG_LAYOUT_MEMBERS


    }
    return compatible;
}

}


