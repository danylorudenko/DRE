#pragma once

#include <cstdint>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorLinear.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\Vector.hpp>

#include <vk_wrapper\queue\Queue.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>
#include <vk_wrapper\pipeline\Dependency.hpp>

namespace VKW
{
class   ImportTable;
class   Queue;
struct  BufferResource;
class   Pipeline;
class   DescriptorSetLayout;
class   PipelineLayout;
class   RenderPass;
class   Framebuffer;
class   Dependency;
class   DescriptorManager;

enum class BindPoint
{
    Graphics,
    Compute
};

enum AttachmentMaskBits
{
    ATTACHMENT_MASK_DEPTH   = 1,
    ATTACHMENT_MASK_STENCIL = 1 << 1,
    ATTACHMENT_MASK_COLOR_0 = 1 << 2,
    ATTACHMENT_MASK_COLOR_1 = 1 << 3,
    ATTACHMENT_MASK_COLOR_2 = 1 << 4,
    ATTACHMENT_MASK_COLOR_3 = 1 << 5,
    ATTACHMENT_MASK_COLOR_4 = 1 << 6
};

enum PolygonModeBits
{
    POLYGON_FILL,
    POLYGON_WIREFRAME
};

using AttachmentMask = std::uint32_t;

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

class Context
    : public NonMovable
    , public NonCopyable
{
public:
    Context(VKW::ImportTable* table, VKW::Queue* queue, DRE::AllocatorLinear* barrierAllocator);
    ~Context();

public:
    inline VKW::Queue* GetParentQueue() const { return m_ParentQueue; }
    inline VKW::CommandList* GetCurrentCommandList() { return m_CurrentCommandList; }

    void ResetDependenciesVectors(DRE::AllocatorLinear* allocator);
    
    void FlushAll();
    void FlushOnlyPending();
    void WriteResourceDependencies();

    void FlushWaitSwapchain(PresentationContext& presentContext);
    void Present(PresentationContext& presentContext);

    VKW::QueueExecutionPoint SyncPoint(std::uint8_t waitCount = 0, VKW::QueueExecutionPoint const* waits = nullptr);
    VKW::QueueExecutionPoint SyncPoint(VKW::QueueExecutionPoint const& wait);

public:
    void CmdDraw(std::uint32_t vertexCount, std::uint32_t instanceCount = 1, std::uint32_t firstVertex = 0, std::uint32_t firstInstance = 0);
    void CmdDrawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount = 1, std::uint32_t firstIndex = 0, std::int32_t vertexOffset = 0, std::uint32_t firstInstance = 0);
    void CmdDispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z);

    void CmdBindVertexBuffer(VKW::BufferResource const* buffer, std::uint32_t offset = 0);
    void CmdBindIndexBuffer(VKW::BufferResource const* buffer, std::uint32_t offset = 0);

    void CmdBindPipeline(BindPoint bindPoint, VKW::Pipeline const* pipeline);
    void CmdBindGraphicsPipeline(VKW::Pipeline const* pipeline);
    void CmdBindComputePipeline(VKW::Pipeline const* pipeline);

    void CmdBindDescriptorSets(
        VKW::PipelineLayout const* layout, BindPoint bindPoint,
        std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
        std::uint32_t dynamicOffsetCount = 0, std::uint32_t const* pDynamicOffsets = nullptr);

    void CmdBindGraphicsDescriptorSets(
        VKW::PipelineLayout const* layout, 
        std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
        std::uint32_t dynamicOffsetCount = 0, std::uint32_t const* pDynamicOffsets = nullptr);

    void CmdBindComputeDescriptorSets(
        VKW::PipelineLayout const* layout,
        std::uint32_t firstSet, std::uint32_t descriptorSetCount, VKW::DescriptorSet const* sets,
        std::uint32_t dynamicOffsetCount = 0, std::uint32_t const* pDynamicOffsets = nullptr);

    void CmdBindGlobalDescriptorSets(VKW::DescriptorManager& descriptorManager, std::uint8_t frameID);

    void CmdSetViewport(std::uint32_t viewportCount, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height);
    void CmdSetScissor(std::uint32_t scissorCount, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height);

#ifndef DRE_COMPILE_FOR_RENDERDOC
    void CmdSetPolygonMode(PolygonModeBits mode);
#endif // DRE_COMPILE_FOR_RENDERDOC

    void CmdPushConstants(VKW::PipelineLayout const* layout, VKW::DescriptorStage stages, std::uint32_t offset, std::uint32_t size, void const* pValues);

    // very heavy
    //void CmdPipelineBarrier(VKW::Dependency& dependency);

    void CmdResourceDependency(VKW::ImageResource const* resource,
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);

    void CmdResourceDependency(VKW::BufferResource const* resource,
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);

    void CmdResourceDependency(VKW::BufferResource const* resource,
        std::uint32_t offset, std::uint32_t size, // offset is added to the base offset of the VKW::BufferResource
        ResourceAccess srcAccess, Stages srcStage,
        ResourceAccess dstAccess, Stages dstStage);


    void CmdBeginRendering(std::uint32_t attachmentCount, VKW::ImageResourceView* const* attachments, VKW::ImageResourceView const* depthAttachment, VKW::ImageResourceView const* stencilAttachment);
    void CmdClearAttachments(AttachmentMask attachments, float* color);
    void CmdClearAttachments(AttachmentMask attachments, std::uint32_t* value);
    void CmdClearAttachments(AttachmentMask attachments, float depth, std::uint32_t stencil);
    void CmdEndRendering();

    void CmdBindVertexBuffer(VKW::BufferResource* vertexBuffer, std::uint32_t offset);
    void CmdBindIndexBuffer(VKW::BufferResource* indexBuffer, std::uint32_t offset, std::uint8_t indexSize = 32);

    void CmdClearColorImage(VKW::ImageResource const* image, float color[4]);
    void CmdClearDepthStencilImage(VKW::ImageResource const* image, float depth, std::uint32_t stencil);

    void CmdCopyImageToImage(VKW::ImageResource const* dst, VKW::ImageResource const* src);
    void CmdCopyImageToBuffer(VKW::BufferResource const* dst, VKW::ImageResource const* src, std::uint32_t bufferOffset);
    void CmdCopyBufferToImage(VKW::ImageResource const* dst, VKW::BufferResource const* src, std::uint32_t bufferOffset);
    void CmdCopyBufferToBuffer(VKW::BufferResource const* dst, std::uint32_t dstOffset, VKW::BufferResource const* scr, std::uint32_t srcOffset, std::uint32_t size);

    void CmdBeginDebugLabel(char const* label);
    void CmdEndDebugLabel();
    void CmdInsertDebugLabel(char const* label);

    void WaitIdle();

private:
    VKW::ImportTable*       m_ImportTable;
    VKW::Queue*             m_ParentQueue;
    VKW::CommandList*       m_CurrentCommandList;

    VkRect2D                m_RenderingRect;

    VKW::Dependency         m_PendingDependency;

};


///////////////////////////////
class _GPU_DEBUG_SCOPE_
{
public:
    _GPU_DEBUG_SCOPE_(Context& context, char const* name)
        : m_Context{ &context }
    {
        m_Context->CmdBeginDebugLabel(name);
    }

    ~_GPU_DEBUG_SCOPE_()
    {
        m_Context->CmdEndDebugLabel();
    }

private:
    Context* m_Context;
};
#define DRE_GPU_SCOPE(Name) VKW::_GPU_DEBUG_SCOPE_ _##Name##_GPU_SCOPE{ context, #Name }
#define DRE_GPU_EVENT(Name) context.CmdInsertDebugLabel(#Name)


}

